/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "nr-mac-scheduler-ue-info-qos.h"

namespace ns3
{
/**
 * \struct pair_hash
 * \brief A hash function for std::pair
 *
 * A hash function for std::pair that combines the hash values of the two elements of the pair.
 * Combine two hash values using bit shift and XOR
 */
struct pair_hash
{
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const
    {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1);
    }
};

/**
 * \struct Weights
 * \brief A hash map for weights
 *
 * A hash map for weights that maps a pair of uint8_t to a double.
 * The pair represents the LCG and the LC ID, and the double represents the weight of the LC.
 */
typedef std::unordered_map<std::pair<uint8_t, uint8_t>, double, pair_hash> Weights;

/**
 * \struct LcObservation
 * \brief A struct for an observation of a flow
 *
 * A struct for an observation of a flow that stores the RNTI, LCG ID, LC ID, QCI, priority, and
 * head-of-line delay of the flow.
 */
struct LcObservation
{
    uint16_t rnti;
    uint8_t lcgId;
    uint8_t lcId;
    uint8_t qci;
    uint8_t priority;
    uint16_t holDelay;
};

/**
 * \ingroup scheduler
 * \brief UE representation for a AI-based scheduler
 *
 * The representation stores the weights of a UE, which are also referred to as actions in the RL
 * model, in response to sending the predefined observation. The observation is a vector of
 * LcObservation, each representing an observation of a flow. In addition to RL-related operations,
 * it updates the metrics in NrMacSchedulerUeInfoQos by inheriting from the NrMacSchedulerUeInfoQos
 * class. In resource allocation per symbol, we can design the reward function of a UE using the QoS
 * metrics.
 *
 * \see LcObservation
 * \see Weights
 * \see NrMacSchedulerUeInfoQos
 */
class NrMacSchedulerUeInfoAi : public NrMacSchedulerUeInfoQos
{
  public:
    /**
     * \brief NrMacSchedulerUeInfoAi constructor
     * \param alpha The fairness metric
     * \param rnti RNTI of the UE
     * \param beamId BeamId of the UE
     * \param fn A function that tells how many RB per RBG
     */
    NrMacSchedulerUeInfoAi(float alpha, uint16_t rnti, BeamId beamId, const GetRbPerRbgFn& fn)
        : NrMacSchedulerUeInfoQos(alpha, rnti, beamId, fn)
    {
    }

    /**
     * \brief Reset DL AI scheduler info
     *
     * Clear the weights for the downlink.
     * It calls also NrMacSchedulerUeInfoQos::ResetDlSchedInfo.
     */
    void ResetDlSchedInfo() override
    {
        m_weightsDl.clear();
        NrMacSchedulerUeInfoQos::ResetDlSchedInfo();
    }

    /**
     * \brief Reset UL AI scheduler info
     *
     * Clear the weights for the uplink.
     * It also calls NrMacSchedulerUeInfoQos::ResetUlSchedInfo.
     */
    void ResetUlSchedInfo() override
    {
        m_weightsUl.clear();
        NrMacSchedulerUeInfoQos::ResetUlSchedInfo();
    }

    /**
     * \brief Get the current observation for downlink
     * \param ue the UE
     * \return a vector of LcObservation with the current observation
     *
     * Get the current observation for downlink by iterating over the active LCs of the UE.
     * The observation is stored in a vector of LcObservation and Each consists of the RNTI,
     * LCG ID, LC ID, QCI, priority, and head-of-line delay of the flow.
     */
    std::vector<LcObservation> GetDlObservation();

    /**
     * \brief Get the current observation for uplink
     * \param ue the UE
     * \return a vector of LcObservation with the current observation
     *
     * Get the current observation for uplink by iterating over the active LCs of the UE.
     * The observation is stored in a vector of LcObservation and Each consists of the RNTI,
     * LCG ID, LC ID, QCI, priority, and head-of-line delay of the flow.
     */
    std::vector<LcObservation> GetUlObservation();

    /**
     * \brief Update the weights for downlink
     * \param weights The weights assigned to a UE
     *
     * Update m_weights by copying the weights assigned to a UE.
     * The weights consist of an unordered_map of (key, value) pairs where the combination of lcgId
     * and lcId is the key, and the weight of the lcId is the value. The higher the weight, the
     * higher the priority of the flow in scheduling.
     */
    void UpdateDlWeights(Weights& weights);

    /**
     * \brief Update the weights for uplink
     * \param weights the weights assigned to a UE
     *
     * Update m_weights by copying the weights assigned to a UE.
     * The weights consist of an unordered_map of (key, value) pairs where the combination of lcgId
     * and lcId is the key, and the weight of the lcId is the value. The higher the weight, the
     * higher the priority of the flow in scheduling.
     */
    void UpdateUlWeights(Weights& weights);

    /**
     * \brief Get the reward for downlink
     * \return The reward for the downlink
     *
     * Calculate the reward for the downlink based on the latest observation and the given weights.
     * The reward is calculated as the sum of the rewards of the active LCs.
     * The reward for an LC \( i \) is calculated as
     * \f$ \text{reward}_{i} = \frac{\text{std::pow}(\text{potentialTput}, \alpha)}{\max(1E-9,
     * \text{avgTput})} \times P_{i} \times \text{HOL}_{i} \f$.
     *
     * \alpha is a fairness metric. \( P \) is the priority associated with the QCI.
     * HOL is the head-of-line delay of the LC.
     * Please note that the throughput is calculated in bit/symbol.
     */
    float GetDlReward();

    /**
     * \brief Get the reward for uplink
     * \return The reward for the uplink
     *
     * Calculate the reward for the uplink based on the latest observation and the given weights.
     * The reward is calculated as the sum of the rewards of the active LCs.
     * The reward for an LC \( i \) is calculated as
     * \f$ \text{reward}_{i} = \frac{\text{std::pow}(\text{potentialTput}, \alpha)}{\max(1E-9,
     * \text{avgTput})} \times P_{i} \times \text{HOL}_{i} \f$.
     *
     * \alpha is a fairness metric. \( P \) is the priority associated with the QCI.
     * HOL is the head-of-line delay of the LC.
     * Please note that the throughput is calculated in bit/symbol.
     */
    float GetUlReward();

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
        double lAiMetric = CalculateDlWeight(lue);
        double rAiMetric = CalculateDlWeight(rue);

        NS_ASSERT_MSG(lAiMetric > 0, "Weight must be greater than zero");
        NS_ASSERT_MSG(rAiMetric > 0, "Weight must be greater than zero");

        return (lAiMetric > rAiMetric);
    }

    /**
     * \brief Calculate the weight of a UE in the downlink
     * \param ue the UE
     * \return the weight of the UE
     *
     * Calculate the weight of a UE in the downlink by iterating over the active LCs of the UE.
     * The weight is calculated as the sum of the weights of the active LCs.
     * The weight of an LC is retrieved from the m_weightsDl map.
     */
    static double CalculateDlWeight(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    {
        double weight = 0;
        auto uePtr = dynamic_cast<NrMacSchedulerUeInfoAi*>(ue.first.get());

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
     * The AI metric is calculated in CalculateUlWeight()
     */
    static bool CompareUeWeightsUl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        double lAiMetric = CalculateUlWeight(lue);
        double rAiMetric = CalculateUlWeight(rue);

        return (lAiMetric > rAiMetric);
    }

    /**
     * \brief Calculate the weight of a UE in the uplink
     * \param ue the UE
     * \return the weight of the UE
     *
     * Calculate the weight of a UE in the uplink by iterating over the active LCs of the UE.
     * The weight is calculated as the sum of the weights of the active LCs.
     * The weight of an LC is retrieved from the m_weightsUl map.
     */
    static double CalculateUlWeight(const NrMacSchedulerNs3::UePtrAndBufferReq& ue)
    {
        double weight = 0;
        auto uePtr = dynamic_cast<NrMacSchedulerUeInfoAi*>(ue.first.get());

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

    Weights m_weightsDl; //!< Weights assigned to each flow for a UE in the downlink
    Weights m_weightsUl; //!< Weights assigned to each flow for a UE in the uplink
};

} // namespace ns3
