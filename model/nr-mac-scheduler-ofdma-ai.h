/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "nr-mac-scheduler-ofdma-qos.h"
#include "nr-mac-scheduler-ue-info-ai.h"

#include <ns3/traced-value.h>

namespace ns3
{

/**
 * \ingroup scheduler
 * \brief The OFDMA scheduler with AI implementation
 */
class NrMacSchedulerOfdmaAi : public NrMacSchedulerOfdmaQos
{
  public:
    /**
     * \brief GetTypeId
     * \return The TypeId of the class
     */
    static TypeId GetTypeId();

    /**
     * \brief NrMacSchedulerOfdmaAi constructor
     */
    NrMacSchedulerOfdmaAi();

    /**
     * \brief Deconstructor
     */
    ~NrMacSchedulerOfdmaAi() override
    {
    }

  protected:
    /**
     * \brief Create an UE representation of the type NrMacSchedulerUeInfoQos
     * \param params parameters
     * \return NrMacSchedulerUeInfo instance
     */
    std::shared_ptr<NrMacSchedulerUeInfo> CreateUeRepresentation(
        const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const override;

    /**
     * \brief Provide the comparison function to order the UE when scheduling DL
     * \return a function that should order two UEs based on their priority: if
     * UE a is less than UE b, it will have an higher priority.
     */
    std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                       const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
    GetUeCompareDlFn() const override;

    /**
     * \brief Provide the comparison function to order the UE when scheduling UL
     * \return a function that should order two UEs based on their priority: if
     * UE a is less than UE b, it will have an higher priority.
     */
    std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                       const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
    GetUeCompareUlFn() const override;

    /**
     * \brief Update the UE representation after a symbol (DL) has been assigned to it
     * \param ue UE to which a symbol has been assigned
     * \param assigned the amount of resources assigned
     * \param totalAssigned the amount of total resources assigned until now
     *
     * After an UE is selected to be eligible for a symbol assignment, its representation
     * should be updated. The subclasses, by implementing this method, update
     * the representation by updating some custom values that reflect the assignment
     * done. These values are the one that, hopefully, are checked by the
     * comparison function returned by GetUeCompareDlFn().
     */
    void AssignedDlResources(const UePtrAndBufferReq& ue,
                             const FTResources& assigned,
                             const FTResources& totalAssigned) const override;

    /**
     * \brief Update the UE representation after a symbol (DL) has been assigned to it
     * \param ue UE to which a symbol has been assigned
     * \param assigned the amount of resources assigned
     * \param totalAssigned the amount of total resources assigned until now
     *
     * After an UE is selected to be eligible for a symbol assignment, its representation
     * should be updated. The subclasses, by implementing this method, update
     * the representation by updating some custom values that reflect the assignment
     * done. These values are the one that, hopefully, are checked by the
     * comparison function returned by GetUeCompareUelFn().
     */
    void AssignedUlResources(const UePtrAndBufferReq& ue,
                             const FTResources& assigned,
                             const FTResources& totalAssigned) const override;

    /**
     * \brief Update the UE representation after a symbol (DL) has been assigned to other UE
     * \param ue UE to which a symbol has not been assigned
     * \param notAssigned the amount of resources not assigned
     * \param totalAssigned the amount of total resources assigned until now
     */
    void NotAssignedDlResources(const UePtrAndBufferReq& ue,
                                const FTResources& notAssigned,
                                const FTResources& totalAssigned) const override;

    /**
     * \brief Update the UE representation after a symbol (UL) has been assigned to other UE
     * \param ue UE to which a symbol has not been assigned
     * \param notAssigned the amount of resources not assigned
     * \param totalAssigned the amount of total resources assigned until now
     */
    void NotAssignedUlResources(const UePtrAndBufferReq& ue,
                                const FTResources& notAssigned,
                                const FTResources& totalAssigned) const override;

    /**
     * \typedef NotifyCb
     * \brief A callback type for notifying with specific parameters.
     *
     * This callback takes the following parameters:
     * - An Observation object representing the observations
     * - A boolean value indicating whether the game is over (true) or not (false)
     * - A float value representing the reward
     * - A string value representing extra information
     * - A pointer to a const NrMacSchedulerOfdmaAi instance
     */
    typedef Callback<void,
                     std::vector<LcObservation>,
                     bool,
                     float,
                     std::string,
                     const NrMacSchedulerOfdmaAi*>
        NotifyCb;
    /**
     * \brief Set the notify callback function.
     * \param notifyCb The callback function to be set
     */
    void SetNotifyCb(NotifyCb notifyCb);

    /**
     * \brief Get UE observations for downlink
     * \param ueVector A vector containing pointers to active UEs and their corresponding buffer
     * requests \return An Observation object representing the observations for all UEs
     */
    std::vector<LcObservation> GetUeObservationsDl(std::vector<UePtrAndBufferReq>& ueVector) const;

    /**
     * \brief Get UE observations for uplink
     * \param ueVector A vector containing pointers to active UEs and their corresponding buffer
     * requests \return An Observation object representing the observations for all UEs
     */
    std::vector<LcObservation> GetUeObservationsUl(std::vector<UePtrAndBufferReq>& ueVector) const;

    /**
     * \brief Check if the downlink game is over
     * \return A boolean value indicating whether the downlink game is over (true) or not (false)
     */
    bool GetIsGameOverDl() const;

    /**
     * \brief Check if the uplink game is over
     * \return A boolean value indicating whether the downlink game is over (true) or not (false)
     */
    bool GetIsGameOverUl() const;

    /**
     * \brief Get rewards for downlink
     * \param ueVector A vector containing pointers to active UEs and their corresponding buffer
     * requests \return A float value representing the calculated rewards
     */
    float GetUeRewardsDl(std::vector<UePtrAndBufferReq>& ueVector) const;

    /**
     * \brief Get rewards for uplink
     * \param ueVector A vector containing pointers to active UEs and their corresponding buffer
     * requests \return A float value representing the calculated rewards
     */
    float GetUeRewardsUl(std::vector<UePtrAndBufferReq>& ueVector) const;

    /**
     * \brief Call the notify callback function in the OpenGymEnv class
     * in the ns3-gym module for downlink
     * \param ueVector A vector containing pointers to active UEs and their corresponding buffer
     * requests
     */
    void CallNotifyDlFn(std::vector<UePtrAndBufferReq>& ueVector) const;

    /**
     * \brief Call the notify callback function in the OpenGymEnv class
     * in the ns3-gym module for uplink
     * \param ueVector A vector containing pointers to active UEs and their corresponding buffer
     * requests
     */
    void CallNotifyUlFn(std::vector<UePtrAndBufferReq>& ueVector) const;

    /**
     * \brief Update weights of all UE for downlink
     * \param ueWeights An unordered map where the key is the UE's RNTI (Radio Network Temporary
     * Identifier) and the value is the UE's weights for all flows \param ueVector A vector
     * containing pointers to active UEs and their corresponding buffer requests
     */
    void UpdateAllUeWeightsDl(std::unordered_map<uint8_t, Weights>& ueWeights,
                              std::vector<UePtrAndBufferReq>& ueVector);

    /**
     * \brief Update weights of all UE for uplink
     * \param ueWeights An unordered map where the key is the UE's RNTI (Radio Network Temporary
     * Identifier) and the value is the UE's weights for all flows \param ueVector A vector
     * containing pointers to active UEs and their corresponding buffer requests
     */
    void UpdateAllUeWeightsUl(std::unordered_map<uint8_t, Weights>& ueWeights,
                              std::vector<UePtrAndBufferReq>& ueVector);

  private:
    float m_alpha{0.0}; //!< PF Fairness index
    double m_timeWindow{
        99.0}; //!< Time window to calculate the throughput. Better to make it an attribute.
    TracedValue<uint32_t> m_tracedValueSymPerBeam;
    NotifyCb m_notifyCb;
};
} // namespace ns3
