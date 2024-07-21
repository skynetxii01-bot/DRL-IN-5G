/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-ue-info-ai.h"

#include <ns3/log.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerUeInfoAI");

Observation
NrMacSchedulerUeInfoAI::GetDlObservation()
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

Observation
NrMacSchedulerUeInfoAI::GetUlObservation()
{
    NS_LOG_FUNCTION(this);
    std::vector<std::vector<double>> observations;
    for (const auto& ueLcg : m_ulLCG)
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
  m_weightsDl = weights;
}

void
NrMacSchedulerUeInfoAI::UpdateUlWeights(Weights& weights)
{
  m_weightsUl = weights;
}

float
NrMacSchedulerUeInfoAI::GetDlReward()
{
    float reward = 0.0;
    for (const auto& ueLcg : m_dlLCG)
    {
        std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

        for (const auto lcId : ueActiveLCs)
        {
            std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);
            reward += std::pow(m_potentialTputDl, m_alpha) /
                      (std::max(1E-9, m_avgTputDl) *
                      LCPtr->m_priority * 
                      LCPtr->m_rlcTransmissionQueueHolDelay);
        }
    }

    return reward;
}

float
NrMacSchedulerUeInfoAI::GetUlReward()
{
    float reward = 0.0;
    for (const auto& ueLcg : m_ulLCG)
    {
        std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

        for (const auto lcId : ueActiveLCs)
        {
            std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);
            reward += std::pow(m_potentialTputUl, m_alpha) /
                      (std::max(1E-9, m_avgTputUl) *
                      LCPtr->m_priority * 
                      LCPtr->m_rlcTransmissionQueueHolDelay);
        }
    }

    return reward;
}

void
NrMacSchedulerUeInfoAI::UpdateDlAIMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                                         double timeWindow,
                                         const Ptr<const NrAmc>& amc)
{
    NS_LOG_FUNCTION(this);
    NrMacSchedulerUeInfoQos::UpdateDlQosMetric(totAssigned, timeWindow, amc);
}

void
NrMacSchedulerUeInfoAI::UpdateUlAIMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                                         double timeWindow,
                                         const Ptr<const NrAmc>& amc)
{
    NS_LOG_FUNCTION(this);
    NrMacSchedulerUeInfoQos::UpdateUlQosMetric(totAssigned, timeWindow, amc);
}

} // namespace ns3
