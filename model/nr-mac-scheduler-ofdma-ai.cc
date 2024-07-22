/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-ofdma-ai.h"

#include <ns3/log.h>

#include <algorithm>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("NrMacSchedulerOfdmaAI");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerOfdmaAI);

TypeId
NrMacSchedulerOfdmaAI::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrMacSchedulerOfdmaAI")
            .SetParent<NrMacSchedulerOfdmaRR>()
            .AddConstructor<NrMacSchedulerOfdmaAI>();
    return tid;
}

NrMacSchedulerOfdmaAI::NrMacSchedulerOfdmaAI()
    : NrMacSchedulerOfdmaRR()
{
}

std::shared_ptr<NrMacSchedulerUeInfo>
NrMacSchedulerOfdmaAI::CreateUeRepresentation(
    const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const
{
    NS_LOG_FUNCTION(this);
    return std::make_shared<NrMacSchedulerUeInfoAI>(
        m_alpha,
        params.m_rnti,
        params.m_beamId,
        std::bind(&NrMacSchedulerOfdmaAI::GetNumRbPerRbg, this));
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaAI::GetUeCompareDlFn() const
{
    return NrMacSchedulerUeInfoAI::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaAI::GetUeCompareUlFn() const
{
    return NrMacSchedulerUeInfoAI::CompareUeWeightsUl;
}

void
NrMacSchedulerOfdmaAI::AssignedDlResources(const UePtrAndBufferReq& ue,
                                           [[maybe_unused]] const FTResources& assigned,
                                           const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateDlAIMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerOfdmaAI::NotAssignedDlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& notAssigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateDlAIMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerOfdmaAI::AssignedUlResources(const UePtrAndBufferReq& ue,
                                           [[maybe_unused]] const FTResources& assigned,
                                           const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateUlAIMetric(totAssigned, m_timeWindow, m_ulAmc);
}

void
NrMacSchedulerOfdmaAI::NotAssignedUlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& notAssigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateUlAIMetric(totAssigned, m_timeWindow, m_ulAmc);
}

void
NrMacSchedulerOfdmaAI::SetNotifyCb(NotifyCb notifyCb)
{
    NS_LOG_FUNCTION(this);
    m_notifyCb = notifyCb;
}

Observation
NrMacSchedulerOfdmaAI::GetUeObservationsDl(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    Observation observations;
    for (const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
        Observation ueObservation = uePtr->GetDlObservation();
        observations.insert(observations.end(), ueObservation.begin(), ueObservation.end());
    }
    return observations;
}

Observation
NrMacSchedulerOfdmaAI::GetUeObservationsUl(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    Observation observations;
    for(const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
        Observation ueObservation = uePtr->GetUlObservation();
        observations.insert(observations.end(), ueObservation.begin(), ueObservation.end());
    }
    return observations;
}

bool
NrMacSchedulerOfdmaAI::GetIsGameOverDl() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
NrMacSchedulerOfdmaAI::GetIsGameOverUl() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

float
NrMacSchedulerOfdmaAI::GetUeRewardsDl(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    float reward = 0.0;
    for(const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
        reward += uePtr->GetDlReward();
    }
    return reward;
}

float
NrMacSchedulerOfdmaAI::GetUeRewardsUl(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    float reward = 0.0;
    for(const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
        reward += uePtr->GetUlReward();
    }
    return reward;
}

void
NrMacSchedulerOfdmaAI::CallNotifyDlFn(std::vector<UePtrAndBufferReq>& ueVector) const 
{
    NS_LOG_FUNCTION(this);
    if(!m_notifyCb.IsNull())
    {
        std::string extraInfo = "";
        m_notifyCb(GetUeObservationsDl(ueVector), GetIsGameOverDl(), GetUeRewardsDl(ueVector), extraInfo, this);
    }
}

void
NrMacSchedulerOfdmaAI::CallNotifyUlFn(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    if(!m_notifyCb.IsNull())
    {
        std::string extraInfo = "";
        m_notifyCb(GetUeObservationsUl(ueVector), GetIsGameOverUl(), GetUeRewardsUl(ueVector), extraInfo, this);
    }
}

void
NrMacSchedulerOfdmaAI::UpdateAllUeWeightsDl(std::unordered_map<uint8_t, Weights>& ueWeights, std::vector<UePtrAndBufferReq>& ueVector)
{
    NS_LOG_FUNCTION(this);
    for (const auto& ue : ueVector)
    {
      auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
      Weights weights = ueWeights.at(uePtr->m_rnti);
      uePtr->UpdateDlWeights(weights);
    }
}

void
NrMacSchedulerOfdmaAI::UpdateAllUeWeightsUl(std::unordered_map<uint8_t, Weights>& ueWeights, std::vector<UePtrAndBufferReq>& ueVector)
{
    NS_LOG_FUNCTION(this);
    for (const auto& ue : ueVector)
    {
      auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
      Weights weights = ueWeights.at(uePtr->m_rnti);
      uePtr->UpdateUlWeights(weights);
    }
}


} // namespace ns3
