#include "slice-env.h"

#ifdef HAVE_OPENGYM

#include "ns3/integer.h"
#include "ns3/nr-ue-mac.h"
#include "ns3/nr-ue-net-device.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrSliceGymEnv");
NS_OBJECT_ENSURE_REGISTERED(NrSliceGymEnv);

namespace
{
constexpr std::array<std::array<int8_t, NrSliceGymEnv::kSliceCount>, NrSliceGymEnv::kActionCount>
    kActionDelta = {{{-1, -1, -1},
                     {-1, -1, 0},
                     {-1, -1, 1},
                     {-1, 0, -1},
                     {-1, 0, 0},
                     {-1, 0, 1},
                     {-1, 1, -1},
                     {-1, 1, 0},
                     {-1, 1, 1},
                     {0, -1, -1},
                     {0, -1, 0},
                     {0, -1, 1},
                     {0, 0, -1},
                     {0, 0, 0},
                     {0, 0, 1},
                     {0, 1, -1},
                     {0, 1, 0},
                     {0, 1, 1},
                     {1, -1, -1},
                     {1, -1, 0},
                     {1, -1, 1},
                     {1, 0, -1},
                     {1, 0, 0},
                     {1, 0, 1},
                     {1, 1, -1},
                     {1, 1, 0},
                     {1, 1, 1}}};

float
Clamp01(double v)
{
    return static_cast<float>(std::max(0.0, std::min(1.0, v)));
}
} // namespace

NrSliceGymEnv::NrSliceGymEnv()
{
    m_observation.fill(0.0F);
}

NrSliceGymEnv::~NrSliceGymEnv() = default;

TypeId
NrSliceGymEnv::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NrSliceGymEnv")
                            .SetParent<OpenGymEnv>()
                            .AddConstructor<NrSliceGymEnv>();
    return tid;
}

void
NrSliceGymEnv::Initialize(const Config& cfg,
                          const Ptr<NrHelper>& nrHelper,
                          const NetDeviceContainer& gnbDevs,
                          const std::array<NetDeviceContainer, kSliceCount>& ueDevsBySlice)
{
    m_cfg = cfg;
    m_nrHelper = nrHelper;
    m_gnbDevs = gnbDevs;
    m_ueDevsBySlice = ueDevsBySlice;
    m_prbAlloc = cfg.initialPrbAlloc;
    m_uesPerSlice = {ueDevsBySlice[EMBB].GetN(), ueDevsBySlice[URLLC].GetN(), ueDevsBySlice[MMTC].GetN()};

    BuildImsiSliceMap();

    m_initialized = true;
    m_stepCount = 0;
    Simulator::Schedule(m_cfg.stepInterval, &NrSliceGymEnv::ScheduleStep, this);
}

void
NrSliceGymEnv::SetFlowMonitor(const Ptr<FlowMonitor>& flowMonitor,
                              const Ptr<Ipv4FlowClassifier>& flowClassifier)
{
    m_flowMonitor = flowMonitor;
    m_flowClassifier = flowClassifier;
}

void
NrSliceGymEnv::BuildImsiSliceMap()
{
    m_imsiToSlice.clear();

    for (uint8_t s = 0; s < kSliceCount; ++s)
    {
        for (uint32_t i = 0; i < m_ueDevsBySlice[s].GetN(); ++i)
        {
            Ptr<NrUeNetDevice> ueDev = DynamicCast<NrUeNetDevice>(m_ueDevsBySlice[s].Get(i));
            if (!ueDev)
            {
                continue;
            }
            m_imsiToSlice[ueDev->GetImsi()] = s;
        }
    }
}

void
NrSliceGymEnv::TryBuildRntiSliceMap()
{
    if (m_rntiMapReady)
    {
        return;
    }

    m_rntiToSlice.clear();
    bool allResolved = true;

    for (uint8_t s = 0; s < kSliceCount; ++s)
    {
        for (uint32_t i = 0; i < m_ueDevsBySlice[s].GetN(); ++i)
        {
            Ptr<NrUeNetDevice> ueDev = DynamicCast<NrUeNetDevice>(m_ueDevsBySlice[s].Get(i));
            if (!ueDev || !ueDev->GetMac(0))
            {
                allResolved = false;
                continue;
            }

            const uint16_t rnti = ueDev->GetMac(0)->GetRnti();
            if (rnti == std::numeric_limits<uint16_t>::max())
            {
                allResolved = false;
                continue;
            }

            m_rntiToSlice[rnti] = s;
        }
    }

    m_rntiMapReady = allResolved && !m_rntiToSlice.empty();
}

void
NrSliceGymEnv::OnSchedulerNotify(
    const std::vector<NrMacSchedulerUeInfoAi::LcObservation>& observations,
    bool isGameOver,
    float reward,
    const std::string& extraInfo,
    const NrMacSchedulerUeInfoAi::UpdateAllUeWeightsFn& updateWeightsFn)
{
    m_lastLcObservations = observations;
    m_gameOver = isGameOver;
    m_reward = reward;
    m_extraInfo = extraInfo;
    m_updateWeightsFn = updateWeightsFn;

    TryBuildRntiSliceMap();
    ApplySliceWeights();
}

Ptr<OpenGymSpace>
NrSliceGymEnv::GetActionSpace()
{
    return CreateObject<OpenGymDiscreteSpace>(kActionCount);
}

Ptr<OpenGymSpace>
NrSliceGymEnv::GetObservationSpace()
{
    const float low = 0.0F;
    const float high = 1.0F;
    std::vector<uint32_t> shape = {kObsSize};
    return CreateObject<OpenGymBoxSpace>(low, high, shape, TypeNameGet<float>());
}

bool
NrSliceGymEnv::GetGameOver()
{
    return m_gameOver;
}

Ptr<OpenGymDataContainer>
NrSliceGymEnv::GetObservation()
{
    std::vector<uint32_t> shape = {kObsSize};
    Ptr<OpenGymBoxContainer<float>> box = CreateObject<OpenGymBoxContainer<float>>(shape);

    for (uint32_t i = 0; i < kObsSize; ++i)
    {
        box->AddValue(m_observation[i]);
    }

    return box;
}

float
NrSliceGymEnv::GetReward()
{
    return m_reward;
}

std::string
NrSliceGymEnv::GetExtraInfo()
{
    return m_extraInfo;
}

bool
NrSliceGymEnv::ExecuteActions(Ptr<OpenGymDataContainer> action)
{
    Ptr<OpenGymDiscreteContainer> discrete = DynamicCast<OpenGymDiscreteContainer>(action);
    if (!discrete)
    {
        NS_LOG_WARN("NrSliceGymEnv received non-discrete action container");
        return false;
    }

    const uint32_t actionId = static_cast<uint32_t>(discrete->GetValue());
    if (actionId >= kActionCount)
    {
        NS_LOG_WARN("Action id out of range: " << actionId);
        return false;
    }

    for (uint32_t s = 0; s < kSliceCount; ++s)
    {
        const int32_t updated = static_cast<int32_t>(m_prbAlloc[s]) + kActionDelta[actionId][s];
        m_prbAlloc[s] = static_cast<uint16_t>(std::max(1, updated));
    }

    EnforceConstraints();
    ApplySliceWeights();
    return true;
}

void
NrSliceGymEnv::EnforceConstraints()
{
    for (auto& prb : m_prbAlloc)
    {
        prb = std::max<uint16_t>(1, prb);
    }

    int32_t diff = static_cast<int32_t>(m_cfg.totalPrbs) -
                   static_cast<int32_t>(m_prbAlloc[0] + m_prbAlloc[1] + m_prbAlloc[2]);

    while (diff != 0)
    {
        if (diff > 0)
        {
            const uint8_t minSlice =
                static_cast<uint8_t>(std::distance(m_prbAlloc.begin(),
                                                   std::min_element(m_prbAlloc.begin(), m_prbAlloc.end())));
            ++m_prbAlloc[minSlice];
            --diff;
            continue;
        }

        const uint8_t maxSlice =
            static_cast<uint8_t>(std::distance(m_prbAlloc.begin(),
                                               std::max_element(m_prbAlloc.begin(), m_prbAlloc.end())));
        if (m_prbAlloc[maxSlice] > 1)
        {
            --m_prbAlloc[maxSlice];
            ++diff;
        }
        else
        {
            break;
        }
    }
}

void
NrSliceGymEnv::ApplySliceWeights()
{
    if (!m_updateWeightsFn)
    {
        return;
    }

    TryBuildRntiSliceMap();

    NrMacSchedulerUeInfoAi::UeWeightsMap weightsMap;

    for (const auto& [rnti16, slice] : m_rntiToSlice)
    {
        if (slice >= kSliceCount)
        {
            continue;
        }

        const double sliceFrac = static_cast<double>(m_prbAlloc[slice]) / m_cfg.totalPrbs;
        const double ueCount = static_cast<double>(std::max<uint32_t>(1, m_uesPerSlice[slice]));
        const double weight = sliceFrac / ueCount;

        const uint8_t rnti8 = static_cast<uint8_t>(std::min<uint16_t>(rnti16, 255));
        weightsMap[rnti8][1] = weight;
    }

    m_updateWeightsFn(weightsMap);
}

uint8_t
NrSliceGymEnv::ResolveSliceFromPort(uint16_t port) const
{
    if (port >= 1000 && port <= 1999)
    {
        return EMBB;
    }
    if (port >= 2000 && port <= 2999)
    {
        return URLLC;
    }
    return MMTC;
}

void
NrSliceGymEnv::AggregateFlowStats()
{
    m_thrMbps.fill(0.0);
    m_latMs.fill(0.0);
    m_queueOcc.fill(0.0);

    if (!m_flowMonitor || !m_flowClassifier)
    {
        return;
    }

    const auto stats = m_flowMonitor->GetFlowStats();
    std::array<uint32_t, kSliceCount> packetsPerSlice{0, 0, 0};

    for (const auto& [flowId, st] : stats)
    {
        Ipv4FlowClassifier::FiveTuple fiveTuple = m_flowClassifier->FindFlow(flowId);
        const uint8_t slice = ResolveSliceFromPort(fiveTuple.destinationPort);

        const double thrMbps =
            (m_cfg.stepInterval.GetSeconds() > 0.0)
                ? (st.rxBytes * 8.0 / 1e6) / std::max(1e-9, m_cfg.stepInterval.GetSeconds())
                : 0.0;
        m_thrMbps[slice] += thrMbps;

        if (st.rxPackets > 0)
        {
            const double meanDelayMs = (st.delaySum.GetSeconds() * 1e3) / st.rxPackets;
            m_latMs[slice] += meanDelayMs;
            ++packetsPerSlice[slice];
        }

        const uint64_t tx = st.txPackets;
        const uint64_t rx = st.rxPackets;
        if (tx > 0)
        {
            m_queueOcc[slice] += static_cast<double>(tx - rx) / static_cast<double>(tx);
        }
    }

    for (uint8_t s = 0; s < kSliceCount; ++s)
    {
        if (packetsPerSlice[s] > 0)
        {
            m_latMs[s] /= packetsPerSlice[s];
        }
        m_queueOcc[s] = std::min(1.0, std::max(0.0, m_queueOcc[s]));
    }
}

void
NrSliceGymEnv::ScheduleStep()
{
    if (!m_initialized)
    {
        return;
    }

    ++m_stepCount;
    AggregateFlowStats();

    for (uint8_t s = 0; s < kSliceCount; ++s)
    {
        m_observation[s] = Clamp01(static_cast<double>(m_prbAlloc[s]) / m_cfg.totalPrbs);
        m_observation[3 + s] = Clamp01(m_thrMbps[s] / m_cfg.maxThrMbps[s]);
        m_observation[6 + s] = Clamp01(m_latMs[s] / m_cfg.maxLatMs[s]);
        m_observation[9 + s] = Clamp01(m_queueOcc[s]);
        m_observation[12 + s] = Clamp01(static_cast<double>(m_uesPerSlice[s]) / m_cfg.maxUes[s]);
    }

    // Reward model using throughput utility, latency utility, Jain fairness, and SLA penalties.
    double thrNormAvg = 0.0;
    double satNormAvg = 0.0;
    uint32_t slaViolations = 0;

    for (uint8_t s = 0; s < kSliceCount; ++s)
    {
        const double thrNorm = std::min(1.0, m_thrMbps[s] / std::max(1e-9, m_cfg.maxThrMbps[s]));
        const bool slaSat = (m_thrMbps[s] >= m_cfg.minThrMbps[s]) && (m_latMs[s] <= m_cfg.maxLatMs[s]);
        const double latNorm = std::min(1.0, m_latMs[s] / std::max(1e-9, m_cfg.maxLatMs[s]));

        thrNormAvg += thrNorm;
        satNormAvg += (slaSat ? 1.0 : 0.0) * (1.0 - latNorm);
        slaViolations += slaSat ? 0 : 1;
    }

    thrNormAvg /= kSliceCount;
    satNormAvg /= kSliceCount;

    const double xsum = std::accumulate(m_thrMbps.begin(), m_thrMbps.end(), 0.0);
    double xsum2 = 0.0;
    for (double x : m_thrMbps)
    {
        xsum2 += x * x;
    }
    const double jain = (xsum2 > 0.0) ? ((xsum * xsum) / (kSliceCount * xsum2)) : 0.0;

    m_reward = static_cast<float>(0.5 * thrNormAvg + 0.3 * satNormAvg + 0.2 * jain -
                                  2.0 * static_cast<double>(slaViolations));

    m_gameOver = Simulator::Now() >= m_cfg.simTime;
    Notify();

    if (!m_gameOver)
    {
        Simulator::Schedule(m_cfg.stepInterval, &NrSliceGymEnv::ScheduleStep, this);
    }
}

} // namespace ns3

#endif // HAVE_OPENGYM
