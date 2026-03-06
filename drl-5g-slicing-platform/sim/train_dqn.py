"""Minimal placeholder for DRL training loop integration.

Replace this with your preferred algorithm (DQN/PPO/A2C) against ns3-gym env.
"""

from __future__ import annotations

import json
from pathlib import Path


def load_active_scenario(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> None:
    scenario_file = Path(__file__).parent / "scenarios" / "active_scenario.json"
    if not scenario_file.exists():
        scenario_file = Path(__file__).parent / "scenarios" / "three-slice-baseline.json"

    scenario = load_active_scenario(scenario_file)
    print("Loaded scenario:")
    print(json.dumps(scenario, indent=2))
    print("TODO: connect to ns3-gym environment and train DRL policy.")


if __name__ == "__main__":
    main()
