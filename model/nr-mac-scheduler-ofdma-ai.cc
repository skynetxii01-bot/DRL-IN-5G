/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-ofdma-ai.h"

#include <ns3/log.h>

#include <algorithm>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("NrMacSchedulerOfdmaAi");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerOfdmaAi);

TypeId
NrMacSchedulerOfdmaAi::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NrMacSchedulerOfdmaAi")
                            .SetParent<NrMacSchedulerOfdmaQos>()
                            .AddConstructor<NrMacSchedulerOfdmaAi>();
    return tid;
}

NrMacSchedulerOfdmaAi::NrMacSchedulerOfdmaAi()
    : NrMacSchedulerOfdmaQos()
{
}

std::shared_ptr<NrMacSchedulerUeInfo>
NrMacSchedulerOfdmaAi::CreateUeRepresentation(
    const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const
{
    NS_LOG_FUNCTION(this);
    return std::make_shared<NrMacSchedulerUeInfoAi>(
        m_alpha,
        params.m_rnti,
        params.m_beamId,
        std::bind(&NrMacSchedulerOfdmaAi::GetNumRbPerRbg, this));
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaAi::GetUeCompareDlFn() const
{
    return NrMacSchedulerUeInfoAi::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaAi::GetUeCompareUlFn() const
{
    return NrMacSchedulerUeInfoAi::CompareUeWeightsUl;
}

void
NrMacSchedulerOfdmaAi::SetNotifyCb(NotifyCb notifyCb)
{
    NS_LOG_FUNCTION(this);
    m_notifyCb = notifyCb;
}

std::vector<LcObservation>
NrMacSchedulerOfdmaAi::GetUeObservationsDl(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    std::vector<LcObservation> observations;
    for (const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAi>(ue.first);
        std::vector<LcObservation> ueObservation = uePtr->GetDlObservation();
        observations.insert(observations.end(), ueObservation.begin(), ueObservation.end());
    }
    return observations;
}

std::vector<LcObservation>
NrMacSchedulerOfdmaAi::GetUeObservationsUl(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    std::vector<LcObservation> observations;
    for (const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAi>(ue.first);
        std::vector<LcObservation> ueObservation = uePtr->GetUlObservation();
        observations.insert(observations.end(), ueObservation.begin(), ueObservation.end());
    }
    return observations;
}

bool
NrMacSchedulerOfdmaAi::GetIsGameOverDl() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
NrMacSchedulerOfdmaAi::GetIsGameOverUl() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

float
NrMacSchedulerOfdmaAi::GetUeRewardsDl(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    float reward = 0.0;
    for (const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAi>(ue.first);
        reward += uePtr->GetDlReward();
    }
    return reward;
}

float
NrMacSchedulerOfdmaAi::GetUeRewardsUl(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    float reward = 0.0;
    for (const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAi>(ue.first);
        reward += uePtr->GetUlReward();
    }
    return reward;
}

void
NrMacSchedulerOfdmaAi::CallNotifyDlFn(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    if (!m_notifyCb.IsNull())
    {
        std::string extraInfo = "";
        m_notifyCb(GetUeObservationsDl(ueVector),
                   GetIsGameOverDl(),
                   GetUeRewardsDl(ueVector),
                   extraInfo,
                   this);
    }
}

void
NrMacSchedulerOfdmaAi::CallNotifyUlFn(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    if (!m_notifyCb.IsNull())
    {
        std::string extraInfo = "";
        m_notifyCb(GetUeObservationsUl(ueVector),
                   GetIsGameOverUl(),
                   GetUeRewardsUl(ueVector),
                   extraInfo,
                   this);
    }
}

void
NrMacSchedulerOfdmaAi::UpdateAllUeWeightsDl(std::unordered_map<uint8_t, Weights>& ueWeights,
                                            std::vector<UePtrAndBufferReq>& ueVector)
{
    NS_LOG_FUNCTION(this);
    for (const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAi>(ue.first);
        Weights weights = ueWeights.at(uePtr->m_rnti);
        uePtr->UpdateDlWeights(weights);
    }
}

void
NrMacSchedulerOfdmaAi::UpdateAllUeWeightsUl(std::unordered_map<uint8_t, Weights>& ueWeights,
                                            std::vector<UePtrAndBufferReq>& ueVector)
{
    NS_LOG_FUNCTION(this);
    for (const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAi>(ue.first);
        Weights weights = ueWeights.at(uePtr->m_rnti);
        uePtr->UpdateUlWeights(weights);
    }
}

} // namespace ns3
