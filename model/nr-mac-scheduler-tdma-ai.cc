/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-tdma-ai.h"

#include <ns3/log.h>

#include <algorithm>
#include <functional>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerTdmaAI");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerTdmaAI);

TypeId
NrMacSchedulerTdmaAI::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrMacSchedulerTdmaAI")
            .SetParent<NrMacSchedulerTdmaRR>()
            .AddConstructor<NrMacSchedulerTdmaAI>();
    return tid;
}

NrMacSchedulerTdmaAI::NrMacSchedulerTdmaAI()
    : NrMacSchedulerTdmaRR()
{
}

std::shared_ptr<NrMacSchedulerUeInfo>
NrMacSchedulerTdmaAI::CreateUeRepresentation(
    const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const
{
    NS_LOG_FUNCTION(this);
    return std::make_shared<NrMacSchedulerUeInfoAI>(
        m_alpha,
        params.m_rnti,
        params.m_beamId,
        std::bind(&NrMacSchedulerTdmaAI::GetNumRbPerRbg, this));
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerTdmaAI::GetUeCompareDlFn() const
{
    return NrMacSchedulerUeInfoAI::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerTdmaAI::GetUeCompareUlFn() const
{
    return NrMacSchedulerUeInfoAI::CompareUeWeightsUl;
}

void
NrMacSchedulerTdmaAI::AssignedDlResources(const UePtrAndBufferReq& ue,
                                          [[maybe_unused]] const FTResources& assigned,
                                          const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateDlAIMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerTdmaAI::NotAssignedDlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& notAssigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateDlAIMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerTdmaAI::AssignedUlResources(const UePtrAndBufferReq& ue,
                                          [[maybe_unused]] const FTResources& assigned,
                                          const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateUlAIMetric(totAssigned, m_timeWindow, m_ulAmc);
}

void
NrMacSchedulerTdmaAI::NotAssignedUlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& notAssigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateUlAIMetric(totAssigned, m_timeWindow, m_ulAmc);
}

void
NrMacSchedulerTdmaAI::SetNotifyCb(NotifyCb notifyCb)
{
    NS_LOG_FUNCTION(this);
    m_notifyCb = notifyCb;
}

Observation
NrMacSchedulerTdmaAI::GetUeObservationsDl(std::vector<UePtrAndBufferReq>& ueVector) const
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
NrMacSchedulerTdmaAI::GetUeObservationsUl(std::vector<UePtrAndBufferReq>& ueVector) const
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
NrMacSchedulerTdmaAI::GetIsGameOverDl() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
NrMacSchedulerTdmaAI::GetIsGameOverUl() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

float
NrMacSchedulerTdmaAI::GetUeRewardsDl(std::vector<UePtrAndBufferReq>& ueVector) const
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
NrMacSchedulerTdmaAI::GetUeRewardsUl(std::vector<UePtrAndBufferReq>& ueVector) const
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
NrMacSchedulerTdmaAI::CallNotifyDlFn(std::vector<UePtrAndBufferReq>& ueVector) const 
{
    NS_LOG_FUNCTION(this);
    if(!m_notifyCb.IsNull())
    {
        std::string extraInfo = "";
        m_notifyCb(GetUeObservationsDl(ueVector), GetIsGameOverDl(), GetUeRewardsDl(ueVector), extraInfo, this);
    }
}

void
NrMacSchedulerTdmaAI::CallNotifyUlFn(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    if(!m_notifyCb.IsNull())
    {
        std::string extraInfo = "";
        m_notifyCb(GetUeObservationsUl(ueVector), GetIsGameOverUl(), GetUeRewardsUl(ueVector), extraInfo, this);
    }
}

void
NrMacSchedulerTdmaAI::UpdateAllUeWeightsDl(std::unordered_map<uint8_t, Weights>& ueWeights, std::vector<UePtrAndBufferReq>& ueVector)
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
NrMacSchedulerTdmaAI::UpdateAllUeWeightsUl(std::unordered_map<uint8_t, Weights>& ueWeights, std::vector<UePtrAndBufferReq>& ueVector)
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
