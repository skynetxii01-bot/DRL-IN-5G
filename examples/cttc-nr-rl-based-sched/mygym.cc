/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

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
    return Create<OpenGymDiscreteSpace>(3);
}

Ptr<OpenGymSpace>
MyGymEnv::GetObservationSpace()
{
    NS_LOG_FUNCTION(this);
    float low = 0.0;
    float high = 100.0;
    std::vector<uint32_t> shape = {
        3,
    };
    std::string dtype = TypeNameGet<float>();
    return Create<OpenGymBoxSpace>(low, high, shape, dtype);
}

bool
MyGymEnv::GetGameOver()
{
    NS_LOG_FUNCTION(this);
    return false;
}

Ptr<OpenGymDataContainer>
MyGymEnv::GetObservation()
{
    NS_LOG_FUNCTION(this);
    std::vector<uint32_t> shape = {
        3,
    };
    Ptr<OpenGymBoxContainer<float>> observation = CreateObject<OpenGymBoxContainer<float>>(shape);
    observation->AddValue(1.0);
    observation->AddValue(2.0);
    observation->AddValue(3.0);
    return observation;
}

float
MyGymEnv::GetReward()
{
    NS_LOG_FUNCTION(this);
    return 1.0;
}

std::string
MyGymEnv::GetExtraInfo()
{
    NS_LOG_FUNCTION(this);
    return "Extra info";
}

bool
MyGymEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
    NS_LOG_FUNCTION(this);
    return true;
}

} // namespace ns3
