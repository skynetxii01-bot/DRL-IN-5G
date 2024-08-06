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

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerTdmaAi");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerTdmaAi);

TypeId
NrMacSchedulerTdmaAi::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NrMacSchedulerTdmaAi")
                            .SetParent<NrMacSchedulerTdmaQos>()
                            .AddConstructor<NrMacSchedulerTdmaAi>();
    return tid;
}

NrMacSchedulerTdmaAi::NrMacSchedulerTdmaAi()
    : NrMacSchedulerTdmaQos()
{
}

std::shared_ptr<NrMacSchedulerUeInfo>
NrMacSchedulerTdmaAi::CreateUeRepresentation(
    const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const
{
    NS_LOG_FUNCTION(this);
    return std::make_shared<NrMacSchedulerUeInfoAi>(
        m_alpha,
        params.m_rnti,
        params.m_beamId,
        std::bind(&NrMacSchedulerTdmaAi::GetNumRbPerRbg, this));
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerTdmaAi::GetUeCompareDlFn() const
{
    if (m_activeDlAi)
    {
        return NrMacSchedulerUeInfoAi::CompareUeWeightsDl;
    }
    return NrMacSchedulerUeInfoQos::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerTdmaAi::GetUeCompareUlFn() const
{
    if (m_activeUlAi)
    {
        return NrMacSchedulerUeInfoAi::CompareUeWeightsUl;
    }
    return NrMacSchedulerUeInfoQos::CompareUeWeightsUl;
}

void
NrMacSchedulerTdmaAi::SetNotifyCbDl(NotifyCb notifyCb)
{
    NS_LOG_FUNCTION(this);
    m_notifyCbDl = notifyCb;
    m_activeDlAi = true;
}

void
NrMacSchedulerTdmaAi::SetNotifyCbUl(NotifyCb notifyCb)
{
    NS_LOG_FUNCTION(this);
    m_notifyCbUl = notifyCb;
    m_activeUlAi = true;
}

std::vector<LcObservation>
NrMacSchedulerTdmaAi::GetUeObservationsDl(std::vector<UePtrAndBufferReq>& ueVector) const
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
NrMacSchedulerTdmaAi::GetUeObservationsUl(std::vector<UePtrAndBufferReq>& ueVector) const
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
NrMacSchedulerTdmaAi::GetIsGameOverDl() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
NrMacSchedulerTdmaAi::GetIsGameOverUl() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

float
NrMacSchedulerTdmaAi::GetUeRewardsDl(std::vector<UePtrAndBufferReq>& ueVector) const
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
NrMacSchedulerTdmaAi::GetUeRewardsUl(std::vector<UePtrAndBufferReq>& ueVector) const
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
NrMacSchedulerTdmaAi::CallNotifyDlFn(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    if (!m_notifyCbDl.IsNull())
    {
        std::string extraInfo = "";
        m_notifyCbDl(GetUeObservationsDl(ueVector),
                     GetIsGameOverDl(),
                     GetUeRewardsDl(ueVector),
                     extraInfo,
                     this);
    }
}

void
NrMacSchedulerTdmaAi::CallNotifyUlFn(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    if (!m_notifyCbUl.IsNull())
    {
        std::string extraInfo = "";
        m_notifyCbUl(GetUeObservationsUl(ueVector),
                     GetIsGameOverUl(),
                     GetUeRewardsUl(ueVector),
                     extraInfo,
                     this);
    }
}

void
NrMacSchedulerTdmaAi::UpdateAllUeWeightsDl(std::unordered_map<uint8_t, Weights>& ueWeights,
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
NrMacSchedulerTdmaAi::UpdateAllUeWeightsUl(std::unordered_map<uint8_t, Weights>& ueWeights,
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
