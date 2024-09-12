/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Seoul National University (SNU)
// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "mygym.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("MyGymEnv");

NS_OBJECT_ENSURE_REGISTERED(MyGymEnv);

MyGymEnv::MyGymEnv()
{
    NS_LOG_FUNCTION(this);
}

MyGymEnv::MyGymEnv(uint32_t numUes)
{
    NS_LOG_FUNCTION(this);
    m_numFlows = numUes;
}

MyGymEnv::~MyGymEnv()
{
    NS_LOG_FUNCTION(this);
}

TypeId
MyGymEnv::GetTypeId()
{
    static TypeId tid = TypeId("MyGymEnv")
                            .SetParent<OpenGymEnv>()
                            .SetGroupName("OpenGym")
                            .AddConstructor<MyGymEnv>();
    return tid;
}

void
MyGymEnv::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

Ptr<OpenGymSpace>
MyGymEnv::GetActionSpace()
{
    NS_LOG_FUNCTION(this);
    float low = 0.0;
    float high = 1.0;
    std::vector<uint32_t> shape = {m_numFlows};
    std::string dtype = TypeNameGet<float>();
    return Create<OpenGymBoxSpace>(low, high, shape, dtype);
}

Ptr<OpenGymSpace>
MyGymEnv::GetObservationSpace()
{
    NS_LOG_FUNCTION(this);
    float low = 0.0;
    float high = 100.0;
    std::vector<uint32_t> shape = {
        m_numFlows,
        4,
    };
    std::string dtype = TypeNameGet<uint16_t>();
    return Create<OpenGymBoxSpace>(low, high, shape, dtype);
}

bool
MyGymEnv::GetGameOver()
{
    NS_LOG_FUNCTION(this);
    return m_gameOver;
}

Ptr<OpenGymDataContainer>
MyGymEnv::GetObservation()
{
    NS_LOG_FUNCTION(this);
    std::vector<uint32_t> shape = {
        m_numFlows,
        4,
    };
    Ptr<OpenGymBoxContainer<uint16_t>> observation =
        CreateObject<OpenGymBoxContainer<uint16_t>>(shape);
    for (auto& obs : m_observation)
    {
        observation->AddValue(obs.rnti);
        observation->AddValue(obs.lcId);
        observation->AddValue(obs.priority);
        observation->AddValue(obs.holDelay);
    }
    return observation;
}

float
MyGymEnv::GetReward()
{
    NS_LOG_FUNCTION(this);
    return m_reward;
}

std::string
MyGymEnv::GetExtraInfo()
{
    NS_LOG_FUNCTION(this);
    return m_extraInfo;
}

bool
MyGymEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
    NS_LOG_FUNCTION(this);
    Ptr<OpenGymBoxContainer<float>> actionBox = DynamicCast<OpenGymBoxContainer<float>>(action);
    std::vector<float> actionData = actionBox->GetData();
    NrMacSchedulerUeInfoAi::UeWeightsMap ueWeightsMap;
    for (uint32_t i = 0; i < m_numFlows; i++)
    {
        if (ueWeightsMap.end() == ueWeightsMap.find(m_observation[i].rnti))
        {
            ueWeightsMap[m_observation[i].rnti] = NrMacSchedulerUeInfoAi::Weights();
        }
        ueWeightsMap[m_observation[i].rnti][m_observation[i].lcId] = actionData[i];
    }
    m_updateAllUeWeightsFn(ueWeightsMap);
    return true;
}

void
MyGymEnv::NotifyCurrentIteration(
    const std::vector<NrMacSchedulerUeInfoAi::LcObservation>& observations,
    bool isGameOver,
    float reward,
    const std::string& extraInfo,
    const NrMacSchedulerUeInfoAi::UpdateAllUeWeightsFn& updateAllUeWeightsFn)
{
    NS_LOG_FUNCTION(this);
    m_observation = observations;
    m_gameOver = isGameOver;
    m_reward = reward;
    m_extraInfo = extraInfo;
    m_updateAllUeWeightsFn = updateAllUeWeightsFn;
    Notify();
}

} // namespace ns3
