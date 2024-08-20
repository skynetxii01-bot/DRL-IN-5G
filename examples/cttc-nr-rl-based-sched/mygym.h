/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

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
    uint32_t m_numUes;
    bool m_gameOver;
    std::vector<NrMacSchedulerUeInfoAi::LcObservation> m_observation;
    float m_reward;
    std::string m_extraInfo;
    NrMacSchedulerUeInfoAi::UpdateAllUeWeightsFn m_updateAllUeWeightsFn;
};
} // namespace ns3
#endif
