/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Seoul National University (SNU)
// Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#if __has_include("ns3/opengym-module.h")
#define HAVE_OPENGYM

#include "ns3/core-module.h"
#include "ns3/nr-module.h"
#include "ns3/opengym-module.h"

namespace ns3
{
/**
 * \brief The Gym environment for the RL-based scheduler
 *
 * This class extends the OpenGymEnv class and implements the Gym environment for the RL-based
 * scheduler. The environment receives observations, gameover status, rewards and extra information
 * from the scheduler and sends them to the RL model via the the OpenGymInterface. The class also
 * receives actions from the RL model and sends them to the scheduler.
 *
 * \see NotifyCurrentIteration
 * \see ExecuteActions
 */
class MyGymEnv : public OpenGymEnv
{
  public:
    MyGymEnv();
    MyGymEnv(uint32_t numUes);
    ~MyGymEnv() override;

    static TypeId GetTypeId();
    void DoDispose() override;

    Ptr<OpenGymSpace> GetActionSpace() override;
    Ptr<OpenGymSpace> GetObservationSpace() override;
    bool GetGameOver() override;
    Ptr<OpenGymDataContainer> GetObservation() override;
    float GetReward() override;
    std::string GetExtraInfo() override;
    bool ExecuteActions(Ptr<OpenGymDataContainer> action) override;

    void NotifyCurrentIteration(
        const std::vector<NrMacSchedulerUeInfoAi::LcObservation>& observations,
        bool isGameOver,
        float reward,
        const std::string& extraInfo,
        const NrMacSchedulerUeInfoAi::UpdateAllUeWeightsFn& updateAllUeWeightsFn);

  private:
    uint32_t m_numFlows;
    bool m_gameOver;
    std::vector<NrMacSchedulerUeInfoAi::LcObservation> m_observation;
    float m_reward;
    std::string m_extraInfo;
    NrMacSchedulerUeInfoAi::UpdateAllUeWeightsFn m_updateAllUeWeightsFn;
};
} // namespace ns3
#endif
