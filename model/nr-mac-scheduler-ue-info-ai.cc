/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-ue-info-ai.h"

#include <ns3/log.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerUeInfoAI");

std::vector<std::vector<double>>
NrMacSchedulerUeInfoAI::GetUeObservation()
{
    NS_LOG_FUNCTION(this);
    std::vector<std::vector<double>> observations;
    for (const auto& ueLcg : m_dlLCG)
        {
            std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

            for (const auto lcId : ueActiveLCs)
            {
                std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);

                std::vector<double> lcObservation;
                lcObservation.push_back(m_rnti);
                lcObservation.push_back(ueLcg.first);
                lcObservation.push_back(lcId);
                lcObservation.push_back(LCPtr->m_qci);
                lcObservation.push_back(LCPtr->m_priority);
                lcObservation.push_back(LCPtr->m_rlcTransmissionQueueHolDelay);

                observations.push_back(lcObservation);
            }
        }
    return observations;
}

void
NrMacSchedulerUeInfoAI::UpdateDlWeights(Weights& weights)
{
  m_dlWeights = weights;
}

void
NrMacSchedulerUeInfoAI::UpdateDlAIMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                                         double timeWindow,
                                         const Ptr<const NrAmc>& amc)
{
    NS_LOG_FUNCTION(this);

    NrMacSchedulerUeInfo::UpdateDlMetric(amc);
    m_currTputDl = static_cast<double>(m_dlTbSize) / (totAssigned.m_sym);
    m_avgTputDl = ((1.0 - (1.0 / static_cast<double>(timeWindow))) * m_lastAvgTputDl) +
                  ((1.0 / timeWindow) * m_currTputDl);

    NS_LOG_DEBUG("Update DL AI Metric for UE "
                 << m_rnti << " DL TBS: " << m_dlTbSize << " Updated currTputDl " << m_currTputDl
                 << " avgTputDl " << m_avgTputDl << " over n. of syms: " << +totAssigned.m_sym
                 << ", last Avg TH Dl " << m_lastAvgTputDl << " total sym assigned "
                 << static_cast<uint32_t>(totAssigned.m_sym)
                 << " updated DL metric: " << m_potentialTputDl / std::max(1E-9, m_avgTputDl));
}

void
NrMacSchedulerUeInfoAI::UpdateUlAIMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                                         double timeWindow,
                                         const Ptr<const NrAmc>& amc)
{
    NS_LOG_FUNCTION(this);

    NrMacSchedulerUeInfo::UpdateUlMetric(amc);

    m_currTputUl = static_cast<double>(m_ulTbSize) / (totAssigned.m_sym);
    m_avgTputUl = ((1.0 - (1.0 / static_cast<double>(timeWindow))) * m_lastAvgTputUl) +
                  ((1.0 / timeWindow) * m_currTputUl);

    NS_LOG_DEBUG("Update UL PF Metric for UE "
                 << m_rnti << " UL TBS: " << m_ulTbSize << " Updated currTputUl " << m_currTputUl
                 << " avgTputUl " << m_avgTputUl << " over n. of syms: " << +totAssigned.m_sym
                 << ", last Avg TH Ul " << m_lastAvgTputUl << " total sym assigned "
                 << static_cast<uint32_t>(totAssigned.m_sym)
                 << " updated UL metric: " << m_potentialTputUl / std::max(1E-9, m_avgTputUl));
}

void
NrMacSchedulerUeInfoAI::CalculatePotentialTPutDl(
    const NrMacSchedulerNs3::FTResources& assignableInIteration,
    const Ptr<const NrAmc>& amc)
{
    NS_LOG_FUNCTION(this);

    uint32_t rbsAssignable = assignableInIteration.m_rbg * GetNumRbPerRbg();
    // Since we compute a new potential throughput every time, there is no harm
    // in initializing it to zero here.
    m_potentialTputDl = 0.0;
    m_potentialTputDl = amc->CalculateTbSize(m_dlMcs, m_dlRank, rbsAssignable);
    m_potentialTputDl /= assignableInIteration.m_sym;

    NS_LOG_INFO("UE " << m_rnti << " potentialTputDl " << m_potentialTputDl << " lastAvgThDl "
                      << m_lastAvgTputDl << " DL PF metric (partial part of AI metric): "
                      << m_potentialTputDl / std::max(1E-9, m_avgTputDl));
}

void
NrMacSchedulerUeInfoAI::CalculatePotentialTPutUl(
    const NrMacSchedulerNs3::FTResources& assignableInIteration,
    const Ptr<const NrAmc>& amc)
{
    NS_LOG_FUNCTION(this);

    uint32_t rbsAssignable = assignableInIteration.m_rbg * GetNumRbPerRbg();
    m_potentialTputUl = amc->CalculateTbSize(m_ulMcs, m_ulRank, rbsAssignable);
    m_potentialTputUl /= assignableInIteration.m_sym;

    NS_LOG_INFO("UE " << m_rnti << " potentialTputUl " << m_potentialTputUl << " lastAvgThUl "
                      << m_lastAvgTputUl << " UL PF metric (partial part of AI metric): "
                      << m_potentialTputUl / std::max(1E-9, m_avgTputUl));
}

} // namespace ns3
