/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "nr-mac-scheduler-ofdma-rr.h"

#include "nr-mac-scheduler-ue-info-ai.h"

#include <ns3/traced-value.h>

namespace ns3
{

/**
 * \ingroup scheduler
 * \brief The OFDMA scheduler with AI implementation
 */
class NrMacSchedulerOfdmaAI : public NrMacSchedulerOfdmaRR
{
  public:
    /**
     * \brief GetTypeId
     * \return The TypeId of the class
     */
    static TypeId GetTypeId();

    /**
     * \brief NrMacSchedulerOfdmaAI constructor
     */
    NrMacSchedulerOfdmaAI();

    /**
     * \brief Deconstructor
     */
    ~NrMacSchedulerOfdmaAI() override
    {
    }

  protected:
    BeamSymbolMap AssignDLRBG(uint32_t symAvail, const ActiveUeMap& activeDl) const override;
    BeamSymbolMap AssignULRBG(uint32_t symAvail, const ActiveUeMap& activeUl) const override;

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
    
    
    typedef Callback<void, std::vector< std::vector<double>>, bool, float, std::string, const NrMacSchedulerOfdmaAI*>
      NotifyCb;
      
    void SetNotifyCb(NotifyCb notifyCb);
    
    /**
     * \brief Get a UE observation
     * \param ue UE to which a rgb has been assigned
     */
    std::vector<std::vector<double>> GetObservation(std::vector<UePtrAndBufferReq>& ueVector) const;

    bool IsGameOver() const;
    
    float UpdateReward() const;

    void CallNotifyFn(std::vector<UePtrAndBufferReq>& ueVector) const;

    void UpdateAllUeWeightsDl(std::unordered_map<uint8_t, Weights>& ueWeights, std::vector<UePtrAndBufferReq>& ueVector);

  private:
    double m_timeWindow{
        99.0}; //!< Time window to calculate the throughput. Better to make it an attribute.
    TracedValue<uint32_t> m_tracedValueSymPerBeam;
    NotifyCb m_notifyCb;

};
} // namespace ns3
