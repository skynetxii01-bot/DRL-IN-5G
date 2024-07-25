/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "nr-mac-scheduler-ue-info-qos.h"

namespace ns3
{
struct pair_hash
{
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const
    {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1); // 비트 시프트와 XOR을 사용하여 두 해시 값을 결합
    }
};

typedef std::unordered_map<std::pair<uint8_t, uint8_t>, double, pair_hash> Weights;
typedef std::vector<std::vector<double>> Observation;

/**
 * \ingroup scheduler
 * \brief UE representation of a scheduler with AI implementation
 */
class NrMacSchedulerUeInfoAI : public NrMacSchedulerUeInfoQos
{
  public:
    /**
     * \brief NrMacSchedulerUeInfoAI constructor
     * \param rnti RNTI of the UE
     * \param beamId BeamId of the UE
     * \param fn A function that tells how many RB per RBG
     */
    NrMacSchedulerUeInfoAI(float alpha, uint16_t rnti, BeamId beamId, const GetRbPerRbgFn& fn)
        : NrMacSchedulerUeInfoQos(alpha, rnti, beamId, fn)
    {
    }

    /**
     * \brief Reset DL AI scheduler info
     *
     * Set the last average throughput to the current average throughput,
     * and zeroes the average throughput as well as the current throughput.
     *
     * It calls also NrMacSchedulerUeInfoAI::ResetDlSchedInfo.
     */
    void ResetDlSchedInfo() override
    {
        m_lastAvgTputDl = m_avgTputDl;
        m_avgTputDl = 0.0;
        m_currTputDl = 0.0;
        m_potentialTputDl = 0.0;
        m_weightsDl.clear();
        NrMacSchedulerUeInfo::ResetDlSchedInfo();
    }

    /**
     * \brief Reset UL AI scheduler info
     *
     * Set the last average throughput to the current average throughput,
     * and zeroes the average throughput as well as the current throughput.
     *
     * It also calls NrMacSchedulerUeInfoAI::ResetUlSchedInfo.
     */
    void ResetUlSchedInfo() override
    {
        m_lastAvgTputUl = m_avgTputUl;
        m_avgTputUl = 0.0;
        m_currTputUl = 0.0;
        m_potentialTputUl = 0.0;
        m_weightsUl.clear();
        NrMacSchedulerUeInfo::ResetUlSchedInfo();
    }

    /**
     * \brief Reset the DL avg Th to the last value
     */
    void ResetDlMetric() override
    {
        NrMacSchedulerUeInfo::ResetDlMetric();
        m_avgTputDl = m_lastAvgTputDl;
    }

    /**
     * \brief Reset the UL avg Th to the last value
     */
    void ResetUlMetric() override
    {
        NrMacSchedulerUeInfo::ResetUlMetric();
        m_avgTputUl = m_lastAvgTputUl;
    }

    /**
     * \brief Get the current observation for downlink
     * \param ue the UE
     * \return a vector of double with the current observation
     */
    Observation GetDlObservation();

    /**
     * \brief Get the current observation for uplink
     * \param ue the UE
     * \return a vector of double with the current observation
     */
    Observation GetUlObservation();

    /**
     * \brief Update the weights for downlink
     * \param weights the weights assigned to the UEs
     *
     * Updates m_weights by copying the weights assigned to the UEs.
     * The weights consists of the unordered_map of the pair <LCG, LC> and the weight.
     */
    void UpdateDlWeights(Weights& weights);

    /**
     * \brief Update the weights for uplink
     * \param weights the weights assigned to the UEs
     *
     * Updates m_weights by copying the weights assigned to the UEs.
     * The weights consists of the unordered_map of the pair <LCG, LC> and the weight.
     */
    void UpdateUlWeights(Weights& weights);

    /**
     * \brief Update the reward for downlink
     * \return the reward for the downlink
     */
    float GetDlReward();

    /**
     * \brief Update the reward for uplink
     * \return the reward for the uplink
     */
    float GetUlReward();

    /**
     * \brief Update the AI metric for downlink
     * \param totAssigned the resources assigned
     * \param timeWindow the time window
     * \param amc a pointer to the AMC
     *
     * Updates m_currTputDl and m_avgTputDl by keeping in consideration
     * the assigned resources (in form of TBS) and the time window.
     * It gets the tbSize by calling NrMacSchedulerUeInfo::UpdateDlMetric.
     */
    void UpdateDlAIMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                          double timeWindow,
                          const Ptr<const NrAmc>& amc);

    /**
     * \brief Update the AI metric for uplink
     * \param totAssigned the resources assigned
     * \param timeWindow the time window
     * \param amc a pointer to the AMC
     *
     * Updates m_currTputUl and m_avgTputUl by keeping in consideration
     * the assigned resources (in form of TBS) and the time window.
     * It gets the tbSize by calling NrMacSchedulerUeInfo::UpdateUlMetric.
     */
    void UpdateUlAIMetric(const NrMacSchedulerNs3::FTResources& totAssigned,
                          double timeWindow,
                          const Ptr<const NrAmc>& amc);

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the AI metric of the left UE is higher than the right UE
     *
     * The AI metric is calculated in CalculateDlWeight()
     */
    static bool CompareUeWeightsDl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        double lAIMetric = CalculateDlWeight(lue);
        double rAIMetric = CalculateDlWeight(rue);

        NS_ASSERT_MSG(lAIMetric > 0, "Weight must be greater than zero");
        NS_ASSERT_MSG(rAIMetric > 0, "Weight must be greater than zero");

        return (lAIMetric > rAIMetric);
    }

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     */
    static double CalculateDlWeight(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    {
        double weight = 0;
        auto uePtr = dynamic_cast<NrMacSchedulerUeInfoAI*>(ue.first.get());

        for (const auto& ueLcg : ue.first->m_dlLCG)
        {
            std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

            for (const auto lcId : ueActiveLCs)
            {
                std::pair<uint8_t, uint8_t> lcgLcPair = std::make_pair(ueLcg.first, lcId);
                auto it = uePtr->m_weightsDl.find(lcgLcPair);

                NS_ASSERT_MSG(it != uePtr->m_weightsDl.end(), "Weight not found");
                weight += it->second;

                NS_ASSERT_MSG(weight > 0, "Weight must be greater than zero");
            }
        }
        return weight;
    }

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the AI metric of the left UE is higher than the right UE
     *
     * The AI metric is calculated as following:
     *
     */
    static bool CompareUeWeightsUl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        double lAIMetric = CalculateUlWeight(lue);
        double rAIMetric = CalculateUlWeight(rue);

        return (lAIMetric > rAIMetric);
    }

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     */
    static double CalculateUlWeight(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    {
        double weight = 0;
        auto uePtr = dynamic_cast<NrMacSchedulerUeInfoAI*>(ue.first.get());

        for (const auto& ueLcg : ue.first->m_ulLCG)
        {
            std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();

            for (const auto lcId : ueActiveLCs)
            {
                std::pair<uint8_t, uint8_t> lcgLcPair = std::make_pair(ueLcg.first, lcId);
                auto it = uePtr->m_weightsUl.find(lcgLcPair);

                NS_ASSERT_MSG(it != uePtr->m_weightsUl.end(), "Weight not found");
                weight += it->second;

                NS_ASSERT_MSG(weight > 0, "Weight must be greater than zero");
            }
        }
        return weight;
    }

    Weights m_weightsDl; //!< Weights assigned to the UEs in downlink
    Weights m_weightsUl; //!< Weights assigned to the UEs in uplink
};

} // namespace ns3
