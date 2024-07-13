/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "nr-mac-scheduler-tdma.h"

#include <functional>
#include <memory>

namespace ns3
{

/**
 * \ingroup scheduler
 * \brief The TDMA scheduler with AI implementation
 */
class NrMacSchedulerTdmaAI : public NrMacSchedulerTdma
{
  public:
    /**
     * \brief GetTypeId
     * \return The TypeId of the class
     */
    static TypeId GetTypeId();

    /**
     * \brief NrMacSchedulerTdma constructor
     */
    NrMacSchedulerTdmaAI();
    /**
     * \brief NrMacSchedulerTdma deconstructor
     */
    ~NrMacSchedulerTdmaAI() override;

  protected:
    BeamSymbolMap AssignDLRBG(uint32_t symAvail, const ActiveUeMap& activeDl) const override;

    BeamSymbolMap AssignULRBG(uint32_t symAvail, const ActiveUeMap& activeUl) const override;

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

  private:
    /**
     * \brief Retrieve the UE vector from an ActiveUeMap
     * \param activeUes UE map
     * \return A Vector of UEs and their buffer requirements (in B)
     *
     * Really used only in TDMA scheduling. Worth moving?
     */
    static std::vector<UePtrAndBufferReq> GetUeVectorFromActiveUeMap(const ActiveUeMap& activeUes);

  private:
    /**
     * \brief //!< Function to notify a successful assignment
     */
    typedef std::function<void(const UePtrAndBufferReq&, const FTResources&, const FTResources&)>
        AfterSuccessfulAssignmentFn;
    /**
     * \brief Function to notify that the UE did not get any resource in one iteration
     */
    typedef std::function<void(const UePtrAndBufferReq&, const FTResources&, const FTResources&)>
        AfterUnsuccessfulAssignmentFn;
    typedef std::function<uint32_t&(const UePtr& ue)> GetRBGFn; //!< Getter for the RBG of an UE
    typedef std::function<uint32_t&(const UePtr& ue)> GetTBSFn; //!< Getter for the TBS of an UE
    typedef std::function<uint8_t&(const UePtr& ue)>
        GetSymFn; //!< Getter for the number of symbols of an UE
    typedef std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                               const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
        CompareUeFn;
    typedef std::function<CompareUeFn()> GetCompareUeFn;

    BeamSymbolMap AssignRBGTDMA(
        uint32_t symAvail,
        const ActiveUeMap& activeUe,
        const std::string& type,
        const GetCompareUeFn& GetCompareFn,
        const GetTBSFn& GetTBSFn,
        const GetRBGFn& GetRBGFn,
        const GetSymFn& GetSymFn,
        const AfterSuccessfulAssignmentFn& SuccessfulAssignmentFn,
        const AfterUnsuccessfulAssignmentFn& UnSuccessfulAssignmentFn) const;

    std::shared_ptr<DciInfoElementTdma> CreateDci(
        PointInFTPlane* spoint,
        const std::shared_ptr<NrMacSchedulerUeInfo>& ueInfo,
        uint32_t tbs,
        DciInfoElementTdma::DciFormat fmt,
        uint32_t mcs,
        uint8_t rank,
        Ptr<const ComplexMatrixArray> precMats,
        uint8_t numSym) const;
    double m_timeWindow{
        99.0}; //!< Time window to calculate the throughput. Better to make it an attribute.
};

} // namespace ns3
