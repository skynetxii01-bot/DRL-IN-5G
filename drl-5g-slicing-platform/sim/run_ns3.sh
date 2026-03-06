#!/usr/bin/env bash
set -euo pipefail

NS3_ROOT="${NS3_ROOT:-$HOME/ns-allinone-3.45/ns-3.45}"
NS3_EXAMPLE="${NS3_EXAMPLE:-gsoc-nr-rl-based-sched}"
SCENARIO_FILE="${SCENARIO_FILE:-$(pwd)/scenarios/active_scenario.json}"

if [[ ! -d "$NS3_ROOT" ]]; then
  echo "NS3_ROOT does not exist: $NS3_ROOT"
  echo "Set NS3_ROOT to your ns-3.45 directory."
  exit 2
fi

if [[ ! -f "$SCENARIO_FILE" ]]; then
  echo "Scenario file not found: $SCENARIO_FILE"
  echo "Create one via backend POST /scenario first."
  exit 3
fi

echo "Using ns-3 root: $NS3_ROOT"
echo "Using scenario: $SCENARIO_FILE"
echo "Running example: $NS3_EXAMPLE"

cd "$NS3_ROOT"

# Replace/add arguments based on your OpenGym-enabled slicing scenario.
./ns3 run "$NS3_EXAMPLE --enableLcLevelQos=1 --ueLevelSchedulerType=Ai"
