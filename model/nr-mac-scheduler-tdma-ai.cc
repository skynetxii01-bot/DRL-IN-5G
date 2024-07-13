/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-tdma-ai.h"

#include "nr-mac-scheduler-ue-info-ai.h"

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
    static TypeId tid = TypeId("ns3::NrMacSchedulerTdmaAI").SetParent<NrMacSchedulerTdma>();
    return tid;
}

NrMacSchedulerTdmaAI::NrMacSchedulerTdmaAI()
{
}

NrMacSchedulerTdmaAI::~NrMacSchedulerTdmaAI()
{
}

std::vector<NrMacSchedulerNs3::UePtrAndBufferReq>
NrMacSchedulerTdmaAI::GetUeVectorFromActiveUeMap(const NrMacSchedulerNs3::ActiveUeMap& activeUes)
{
    std::vector<UePtrAndBufferReq> ueVector;
    for (const auto& el : activeUes)
    {
        uint64_t size = ueVector.size();
        GetSecond GetUeVector;
        for (const auto& ue : GetUeVector(el))
        {
            ueVector.emplace_back(ue);
        }
        NS_ASSERT(size + GetUeVector(el).size() == ueVector.size());
    }
    return ueVector;
}

/**
 * \brief Assign the available RBG in a TDMA fashion
 * \param symAvail Number of available symbols
 * \param activeUe active flows and UE
 * \param type String representing the type of allocation currently in act (DL or UL)
 * \param GetCompareFn Function to call to compare UEs during assignment
 * \param GetTBSFn Function to call to get a reference of the UL or DL TBS
 * \param GetRBGFn Function to call to get a reference of the UL or DL RBG
 * \param GetSymFn Function to call to get a reference of the UL or DL symbols
 * \param SuccessfulAssignmentFn Function to call one time for the UE that got the resources
 * assigned in one iteration \param UnSuccessfulAssignmentFn Function to call for the UEs that did
 * not get anything in one iteration
 *
 * \return a map between the beam and the symbols assigned to each one
 *
 * The algorithm redistributes the number of symbols to all the UEs. The
 * pseudocode is the following:
 * <pre>
 * for (ue : activeUe):
 *    BeforeSchedFn (ue);
 *
 * while symbols > 0:
 *    sort (ueVector);
 *    GetRBGFn(ueVector.first()) += BandwidthInRBG();
 *    symbols--;
 *    SuccessfulAssignmentFn (ueVector.first());
 *    for each ue that did not get anything assigned:
 *        UnSuccessfulAssignmentFn (ue);
 * </pre>
 *
 * To sort the UEs, the method uses the function returned by GetUeCompareDlFn().
 * Two fairness helper are hard-coded in the method: the first one is avoid
 * to assign resources to UEs that already have their buffer requirement covered,
 * and the other one is avoid to assign symbols when all the UEs have their
 * requirements covered.
 *
 * The distribution of each symbol is called 'iteration' in other part of the
 * class documentation.
 *
 * The function, thanks to the callback parameters, can be adapted to do
 * a UL or DL allocation. Please make sure the callbacks return references
 * (or no effects will be seen on the caller).
 *
 * \see BeforeDlSched
 */
NrMacSchedulerTdmaAI::BeamSymbolMap
NrMacSchedulerTdmaAI::AssignRBGTDMA(
    uint32_t symAvail,
    const ActiveUeMap& activeUe,
    const std::string& type,
    const GetCompareUeFn& GetCompareFn,
    const GetTBSFn& GetTBSFn,
    const GetRBGFn& GetRBGFn,
    const GetSymFn& GetSymFn,
    const AfterSuccessfulAssignmentFn& SuccessfulAssignmentFn,
    const AfterUnsuccessfulAssignmentFn& UnSuccessfulAssignmentFn) const
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Assigning RBG in " << type << ", # beams active flows: " << activeUe.size()
                                     << ", # sym: " << symAvail);

    // Create vector of UE (without considering the beam)
    std::vector<UePtrAndBufferReq> ueVector = GetUeVectorFromActiveUeMap(activeUe);

    // Distribute the symbols following the selected behaviour among UEs
    uint32_t resources = symAvail;
    FTResources assigned(0, 0);

    const std::vector<uint8_t> notchedRBGsMask =
        type == "DL" ? GetDlNotchedRbgMask() : GetUlNotchedRbgMask();
    int zeroes = std::count(notchedRBGsMask.begin(), notchedRBGsMask.end(), 0);
    uint32_t numOfAssignableRbgs = GetBandwidthInRbg() - zeroes;
    NS_ASSERT(numOfAssignableRbgs > 0);

    while (resources > 0)
    {
        GetFirst GetUe;

        auto schedInfoIt = ueVector.begin();

        std::sort(ueVector.begin(), ueVector.end(), GetCompareFn());

        // Ensure fairness: pass over UEs which already has enough resources to transmit
        while (schedInfoIt != ueVector.end())
        {
            uint32_t bufQueueSize = schedInfoIt->second;

            if (GetTBSFn(GetUe(*schedInfoIt)) >= std::max(bufQueueSize, 10U))
            {
                NS_LOG_INFO("UE " << GetUe(*schedInfoIt)->m_rnti << " TBS "
                                  << GetTBSFn(GetUe(*schedInfoIt)) << " queue " << bufQueueSize
                                  << ", passing");
                schedInfoIt++;
            }
            else
            {
                break;
            }
        }

        // In the case that all the UE already have their requirements fulfilled,
        // then stop the assignment
        if (schedInfoIt == ueVector.end())
        {
            NS_LOG_INFO("All the UE already have their resources allocated. Skipping the beam");
            break;
        }

        // Assign 1 entire symbol (full RBG) to the selected UE and to the total
        // resources assigned count
        GetRBGFn(GetUe(*schedInfoIt)) += numOfAssignableRbgs;
        assigned.m_rbg += numOfAssignableRbgs;

        GetSymFn(GetUe(*schedInfoIt)) += 1;
        assigned.m_sym += 1;

        // subtract 1 SYM from the number of sym available for the while loop
        resources -= 1;

        // Update metrics for the successful UE
        NS_LOG_DEBUG("Assigned " << numOfAssignableRbgs << " " << type << " RBG (= 1 SYM) to UE "
                                 << GetUe(*schedInfoIt)->m_rnti
                                 << " total assigned up to now: " << GetRBGFn(GetUe(*schedInfoIt))
                                 << " that corresponds to " << assigned.m_rbg);
        SuccessfulAssignmentFn(*schedInfoIt, FTResources(numOfAssignableRbgs, 1), assigned);

        // Update metrics for the unsuccessful UEs (who did not get any resource in this iteration)
        for (auto& ue : ueVector)
        {
            if (GetUe(ue)->m_rnti != GetUe(*schedInfoIt)->m_rnti)
            {
                UnSuccessfulAssignmentFn(ue, FTResources(numOfAssignableRbgs, 1), assigned);
            }
        }
    }

    // Count the number of assigned symbol of each beam.
    NrMacSchedulerTdmaAI::BeamSymbolMap ret;
    for (const auto& el : activeUe)
    {
        uint32_t symOfBeam = 0;
        for (const auto& ue : el.second)
        {
            symOfBeam += GetRBGFn(ue.first) / numOfAssignableRbgs;
        }
        ret.insert(std::make_pair(el.first, symOfBeam));
    }
    return ret;
}

/**
 * \brief Assign the available DL RBG to the UEs
 * \param symAvail Number of available symbols
 * \param activeDl active DL flows and UE
 * \return a map between the beam and the symbols assigned to each one
 *
 * The function will prepare all the needed callbacks to return UE DL parameters
 * (e.g., the DL TBS, the DL RBG) and then will call NrMacSchedulerTdmaAI::AssignRBGTDMA.
 */
NrMacSchedulerTdmaAI::BeamSymbolMap
NrMacSchedulerTdmaAI::AssignDLRBG(uint32_t symAvail, const ActiveUeMap& activeDl) const
{
    NS_LOG_FUNCTION(this);

    AfterSuccessfulAssignmentFn SuccFn = std::bind(&NrMacSchedulerTdmaAI::AssignedDlResources,
                                                   this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
    AfterUnsuccessfulAssignmentFn UnSuccFn =
        std::bind(&NrMacSchedulerTdmaAI::NotAssignedDlResources,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3);
    GetCompareUeFn compareFn = std::bind(&NrMacSchedulerTdmaAI::GetUeCompareDlFn, this);

    GetTBSFn GetTbs = &NrMacSchedulerUeInfo::GetDlTBS;
    GetRBGFn GetRBG = &NrMacSchedulerUeInfo::GetDlRBG;
    GetSymFn GetSym = &NrMacSchedulerUeInfo::GetDlSym;

    return AssignRBGTDMA(symAvail,
                         activeDl,
                         "DL",
                         compareFn,
                         GetTbs,
                         GetRBG,
                         GetSym,
                         SuccFn,
                         UnSuccFn);
}

/**
 * \brief Assign the available UL RBG to the UEs
 * \param symAvail Number of available symbols
 * \param activeUl active DL flows and UE
 * \return a map between the beam and the symbols assigned to each one
 *
 * The function will prepare all the needed callbacks to return UE UL parameters
 * (e.g., the UL TBS, the UL RBG) and then will call NrMacSchedulerTdmaAI::AssignRBGTDMA.
 */
NrMacSchedulerTdmaAI::BeamSymbolMap
NrMacSchedulerTdmaAI::AssignULRBG(uint32_t symAvail, const ActiveUeMap& activeUl) const
{
    NS_LOG_FUNCTION(this);
    AfterSuccessfulAssignmentFn SuccFn = std::bind(&NrMacSchedulerTdmaAI::AssignedUlResources,
                                                   this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
    GetCompareUeFn compareFn = std::bind(&NrMacSchedulerTdmaAI::GetUeCompareUlFn, this);
    AfterUnsuccessfulAssignmentFn UnSuccFn =
        std::bind(&NrMacSchedulerTdmaAI::NotAssignedUlResources,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3);
    GetTBSFn GetTbs = &NrMacSchedulerUeInfo::GetUlTBS;
    GetRBGFn GetRBG = &NrMacSchedulerUeInfo::GetUlRBG;
    GetSymFn GetSym = &NrMacSchedulerUeInfo::GetUlSym;

    return AssignRBGTDMA(symAvail,
                         activeUl,
                         "UL",
                         compareFn,
                         GetTbs,
                         GetRBG,
                         GetSym,
                         SuccFn,
                         UnSuccFn);
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

/**
 * \brief Create a DCI with the parameters specified as input
 * \param spoint starting point
 * \param ueInfo ue representation
 * \param tbs Transport Block Size
 * \param fmt Format of the DCI (UL or DL)
 * \param mcs MCS
 * \param numSym Number of symbols
 * \return a pointer to the newly created DCI
 *
 * Creates a TDMA DCI (a DCI with all the resource block assigned for the
 * specified number of symbols)
 */
std::shared_ptr<DciInfoElementTdma>
NrMacSchedulerTdmaAI::CreateDci(NrMacSchedulerNs3::PointInFTPlane* spoint,
                                const std::shared_ptr<NrMacSchedulerUeInfo>& ueInfo,
                                uint32_t tbs,
                                DciInfoElementTdma::DciFormat fmt,
                                uint32_t mcs,
                                uint8_t rank,
                                Ptr<const ComplexMatrixArray> precMats,
                                uint8_t numSym) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(tbs > 0);
    NS_ASSERT(numSym > 0);

    std::shared_ptr<DciInfoElementTdma> dci =
        std::make_shared<DciInfoElementTdma>(ueInfo->m_rnti,
                                             fmt,
                                             spoint->m_sym,
                                             numSym,
                                             mcs,
                                             rank,
                                             precMats,
                                             tbs,
                                             1,
                                             0,
                                             DciInfoElementTdma::DATA,
                                             GetBwpId(),
                                             GetTpc());

    std::vector<uint8_t> rbgAssigned =
        fmt == DciInfoElementTdma::DL ? GetDlNotchedRbgMask() : GetUlNotchedRbgMask();

    if (rbgAssigned.empty())
    {
        rbgAssigned = std::vector<uint8_t>(GetBandwidthInRbg(), 1);
    }

    NS_ASSERT(rbgAssigned.size() == GetBandwidthInRbg());

    dci->m_rbgBitmask = std::move(rbgAssigned);

    std::ostringstream oss;
    for (auto& x : dci->m_rbgBitmask)
    {
        oss << std::to_string(x) << " ";
    }

    NS_LOG_INFO("UE " << ueInfo->m_rnti << " assigned RBG from " << spoint->m_rbg << " with mask "
                      << oss.str() << " for " << static_cast<uint32_t>(numSym) << " SYM ");

    NS_ASSERT(std::count(dci->m_rbgBitmask.begin(), dci->m_rbgBitmask.end(), 0) !=
              GetBandwidthInRbg());

    return dci;
}

} // namespace ns3
