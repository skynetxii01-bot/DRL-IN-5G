/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-ofdma-ai.h"

#include "nr-mac-scheduler-ue-info-ai.h"

#include <ns3/log.h>

#include <algorithm>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("NrMacSchedulerOfdmaAI");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerOfdmaAI);

TypeId
NrMacSchedulerOfdmaAI::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrMacSchedulerOfdmaAI")
            .SetParent<NrMacSchedulerOfdmaRR>()
            .AddConstructor<NrMacSchedulerOfdmaAI>();
    return tid;
}

NrMacSchedulerOfdmaAI::NrMacSchedulerOfdmaAI()
    : NrMacSchedulerOfdmaRR()
{
}

/**
 * \brief Assign the available DL RBG to the UEs
 * \param symAvail Available symbols
 * \param activeDl Map of active UE and their beams
 * \return a map between beams and the symbol they need
 *
 * The algorithm redistributes the frequencies to all the UEs inside a beam.
 * The pre-requisite is to calculate the symbols for each beam, done with
 * the function GetSymPerBeam().
 * The pseudocode is the following (please note that sym_of_beam is a value
 * returned by the GetSymPerBeam() function):
 * <pre>
 * while frequencies > 0:
 *    sort (ueVector);
 *    ueVector.first().m_dlRBG += 1 * sym_of_beam;
 *    frequencies--;
 *    UpdateUeDlMetric (ueVector.first());
 * </pre>
 *
 * To sort the UEs, the method uses the function returned by GetUeCompareDlFn().
 * Two fairness helper are hard-coded in the method: the first one is avoid
 * to assign resources to UEs that already have their buffer requirement covered,
 * and the other one is avoid to assign symbols when all the UEs have their
 * requirements covered.
 */
NrMacSchedulerNs3::BeamSymbolMap
NrMacSchedulerOfdmaAI::AssignDLRBG(uint32_t symAvail, const ActiveUeMap& activeDl) const
{
    NS_LOG_FUNCTION(this);

    NS_LOG_DEBUG("# beams active flows: " << activeDl.size() << ", # sym: " << symAvail);

    GetFirst GetBeamId;
    GetSecond GetUeVector;
    BeamSymbolMap symPerBeam = GetSymPerBeam(symAvail, activeDl);

    // Iterate through the different beams
    for (const auto& el : activeDl)
    {
        // Distribute the RBG evenly among UEs of the same beam
        uint32_t beamSym = symPerBeam.at(GetBeamId(el));
        uint32_t rbgAssignable = 1 * beamSym;
        std::vector<UePtrAndBufferReq> ueVector;
        FTResources assigned(0, 0);
        const std::vector<uint8_t> dlNotchedRBGsMask = GetDlNotchedRbgMask();
        uint32_t resources = !dlNotchedRBGsMask.empty()
                                 ? std::count(dlNotchedRBGsMask.begin(), dlNotchedRBGsMask.end(), 1)
                                 : GetBandwidthInRbg();
        NS_ASSERT(resources > 0);

        for (const auto& ue : GetUeVector(el))
        {
            ueVector.emplace_back(ue);
        }

        while (resources > 0)
        {
            CallNotifyFn(ueVector);
            GetFirst GetUe;
            std::sort(ueVector.begin(), ueVector.end(), GetUeCompareDlFn());
            auto schedInfoIt = ueVector.begin();

            // Ensure fairness: pass over UEs which already has enough resources to transmit
            while (schedInfoIt != ueVector.end())
            {
                uint32_t bufQueueSize = schedInfoIt->second;
                if (GetUe(*schedInfoIt)->m_dlTbSize >= std::max(bufQueueSize, 10U))
                {
                    schedInfoIt++;
                }
                else
                {
                    break;
                }
            }

            // In the case that all the UE already have their requirements fulfilled,
            // then stop the beam processing and pass to the next
            if (schedInfoIt == ueVector.end())
            {
                break;
            }

            // Assign 1 RBG for each available symbols for the beam,
            // and then update the count of available resources
            GetUe(*schedInfoIt)->m_dlRBG += rbgAssignable;
            assigned.m_rbg += rbgAssignable;

            GetUe(*schedInfoIt)->m_dlSym = beamSym;
            assigned.m_sym = beamSym;

            resources -= 1; // Resources are RBG, so they do not consider the beamSym

            // Update metrics
            NS_LOG_DEBUG("Assigned " << rbgAssignable << " DL RBG, spanned over " << beamSym
                                     << " SYM, to UE " << GetUe(*schedInfoIt)->m_rnti);
            AssignedDlResources(*schedInfoIt, FTResources(rbgAssignable, beamSym), assigned);

            // Update metrics for the unsuccessful UEs (who did not get any resource in this
            // iteration)
            for (auto& ue : ueVector)
            {
                if (GetUe(ue)->m_rnti != GetUe(*schedInfoIt)->m_rnti)
                {
                    NotAssignedDlResources(ue, FTResources(rbgAssignable, beamSym), assigned);
                }
            }
        }
    }

    return symPerBeam;
}

NrMacSchedulerNs3::BeamSymbolMap
NrMacSchedulerOfdmaAI::AssignULRBG(uint32_t symAvail, const ActiveUeMap& activeUl) const
{
    NS_LOG_FUNCTION(this);

    NS_LOG_DEBUG("# beams active flows: " << activeUl.size() << ", # sym: " << symAvail);

    GetFirst GetBeamId;
    GetSecond GetUeVector;
    BeamSymbolMap symPerBeam = GetSymPerBeam(symAvail, activeUl);

    // Iterate through the different beams
    for (const auto& el : activeUl)
    {
        // Distribute the RBG evenly among UEs of the same beam
        uint32_t beamSym = symPerBeam.at(GetBeamId(el));
        uint32_t rbgAssignable = 1 * beamSym;
        std::vector<UePtrAndBufferReq> ueVector;
        FTResources assigned(0, 0);
        const std::vector<uint8_t> ulNotchedRBGsMask = GetUlNotchedRbgMask();
        uint32_t resources = !ulNotchedRBGsMask.empty()
                                 ? std::count(ulNotchedRBGsMask.begin(), ulNotchedRBGsMask.end(), 1)
                                 : GetBandwidthInRbg();
        NS_ASSERT(resources > 0);

        for (const auto& ue : GetUeVector(el))
        {
            ueVector.emplace_back(ue);
        }

        for (auto& ue : ueVector)
        {
            BeforeUlSched(ue, FTResources(rbgAssignable * beamSym, beamSym));
        }

        while (resources > 0)
        {
            GetFirst GetUe;
            std::sort(ueVector.begin(), ueVector.end(), GetUeCompareUlFn());
            auto schedInfoIt = ueVector.begin();

            // Ensure fairness: pass over UEs which already has enough resources to transmit
            while (schedInfoIt != ueVector.end())
            {
                uint32_t bufQueueSize = schedInfoIt->second;
                if (GetUe(*schedInfoIt)->m_ulTbSize >= std::max(bufQueueSize, 12U))
                {
                    schedInfoIt++;
                }
                else
                {
                    break;
                }
            }

            // In the case that all the UE already have their requirements fulfilled,
            // then stop the beam processing and pass to the next
            if (schedInfoIt == ueVector.end())
            {
                break;
            }

            // Assign 1 RBG for each available symbols for the beam,
            // and then update the count of available resources
            GetUe(*schedInfoIt)->m_ulRBG += rbgAssignable;
            assigned.m_rbg += rbgAssignable;

            GetUe(*schedInfoIt)->m_ulSym = beamSym;
            assigned.m_sym = beamSym;

            resources -= 1; // Resources are RBG, so they do not consider the beamSym

            // Update metrics
            NS_LOG_DEBUG("Assigned " << rbgAssignable << " UL RBG, spanned over " << beamSym
                                     << " SYM, to UE " << GetUe(*schedInfoIt)->m_rnti);
            AssignedUlResources(*schedInfoIt, FTResources(rbgAssignable, beamSym), assigned);

            // Update metrics for the unsuccessful UEs (who did not get any resource in this
            // iteration)
            for (auto& ue : ueVector)
            {
                if (GetUe(ue)->m_rnti != GetUe(*schedInfoIt)->m_rnti)
                {
                    NotAssignedUlResources(ue, FTResources(rbgAssignable, beamSym), assigned);
                }
            }
        }
    }

    return symPerBeam;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaAI::GetUeCompareDlFn() const
{
    return NrMacSchedulerUeInfoAI::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaAI::GetUeCompareUlFn() const
{
    return NrMacSchedulerUeInfoAI::CompareUeWeightsUl;
}

void
NrMacSchedulerOfdmaAI::AssignedDlResources(const UePtrAndBufferReq& ue,
                                           [[maybe_unused]] const FTResources& assigned,
                                           const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateDlAIMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerOfdmaAI::NotAssignedDlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& notAssigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateDlAIMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerOfdmaAI::AssignedUlResources(const UePtrAndBufferReq& ue,
                                           [[maybe_unused]] const FTResources& assigned,
                                           const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateUlAIMetric(totAssigned, m_timeWindow, m_ulAmc);
}

void
NrMacSchedulerOfdmaAI::NotAssignedUlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& notAssigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
    uePtr->UpdateUlAIMetric(totAssigned, m_timeWindow, m_ulAmc);
}

std::vector<std::vector<double>>
NrMacSchedulerOfdmaAI::GetObservation(std::vector<UePtrAndBufferReq>& ueVector) const
{
    NS_LOG_FUNCTION(this);
    std::vector<std::vector<double>> observations;
    for (const auto& ue : ueVector)
    {
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoAI>(ue.first);
        std::vector<std::vector<double>> ueObservation = uePtr->GetUeObservation();
        observations.insert(observations.end(), ueObservation.begin(), ueObservation.end());
    }
    return observations;
}

bool
NrMacSchedulerOfdmaAI::IsGameOver() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

float
NrMacSchedulerOfdmaAI::UpdateReward() const
{
    NS_LOG_FUNCTION(this);
    float reward = 0.0;
    return reward;
}

void
NrMacSchedulerOfdmaAI::CallNotifyFn(std::vector<UePtrAndBufferReq>& ueVector) const 
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(!m_updateCurrentStateCb.IsNull(), "Transfer observation function is not set");
    NS_ASSERT_MSG(!m_notifyCb.IsNull(), "Notify function is not set");
    m_updateCurrentStateCb(GetObservation(ueVector), IsGameOver(), UpdateReward(), "");
    m_notifyCb();
}


} // namespace ns3
