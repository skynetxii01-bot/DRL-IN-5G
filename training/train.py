#!/usr/bin/env python3
"""Training entrypoint (Phase 4: DQN only)."""

from __future__ import annotations

import argparse
import json
import os
import random
import sys
from pathlib import Path
from typing import Any, Dict

import numpy as np
import torch
import yaml

# Allow direct execution: python training/train.py
PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from agents.dqn.dqn_agent import DqnAgent, DqnConfig
from envs.slice_gym_env import SLICE_NAMES, SliceGymEnv


def set_seed(seed: int) -> None:
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def load_config(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def compute_sla_rate(decoded_obs: Dict[str, Dict[str, float]], cfg: Dict[str, Any]) -> float:
    max_thr = cfg["env"]["max_thr_mbps"]
    min_thr = cfg["env"]["min_thr_mbps"]
    max_lat = cfg["env"]["max_lat_ms"]

    sat = 0
    for s in SLICE_NAMES:
        thr_mbps = float(decoded_obs["throughput"][s]) * float(max_thr[s])
        lat_ms = float(decoded_obs["latency"][s]) * float(max_lat[s])
        if thr_mbps >= float(min_thr[s]) and lat_ms <= float(max_lat[s]):
            sat += 1
    return sat / len(SLICE_NAMES)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--agent", type=str, default="dqn", choices=["dqn"])
    parser.add_argument("--port", type=int, default=5555)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--episodes", type=int, default=None)
    parser.add_argument("--config", type=str, default="configs/config.yaml")
    parser.add_argument("--device", type=str, default="cuda" if torch.cuda.is_available() else "cpu")
    args = parser.parse_args()

    cfg = load_config(Path(args.config))
    if args.episodes is None:
        args.episodes = int(cfg["train"]["episodes"])

    set_seed(args.seed)

    env = SliceGymEnv(port=args.port, sim_seed=args.seed, start_sim=False)
    device = torch.device(args.device)

    dqn_cfg = DqnConfig(
        obs_dim=15,
        action_dim=27,
        lr=float(cfg["dqn"]["lr"]),
        gamma=float(cfg["dqn"]["gamma"]),
        batch_size=int(cfg["dqn"]["batch_size"]),
        buffer_size=int(cfg["dqn"]["buffer_size"]),
        tau=float(cfg["dqn"]["tau"]),
        eps_start=float(cfg["dqn"]["eps_start"]),
        eps_end=float(cfg["dqn"]["eps_end"]),
        eps_decay_steps=int(cfg["dqn"]["eps_decay_steps"]),
        grad_clip=float(cfg["dqn"]["grad_clip"]),
    )
    agent = DqnAgent(dqn_cfg, device)

    log_path = Path("results/logs/dqn_log.jsonl")
    model_dir = Path("results/models")
    os.makedirs(log_path.parent, exist_ok=True)
    os.makedirs(model_dir, exist_ok=True)

    max_steps = int(cfg["train"]["max_steps"])
    save_every = int(cfg["train"]["save_every"])

    with log_path.open("a", encoding="utf-8") as logf:
        for ep in range(1, args.episodes + 1):
            obs, info = env.reset(seed=args.seed + ep)
            ep_reward = 0.0
            step_count = 0
            decoded = info.get("decoded_obs", None)

            for _ in range(max_steps):
                action = agent.act(obs, explore=True)
                next_obs, reward, done, truncated, info = env.step(action)
                terminal = bool(done or truncated)

                agent.store(obs, action, reward, next_obs, terminal)
                agent.train_step()

                ep_reward += float(reward)
                step_count += 1
                obs = next_obs
                decoded = info.get("decoded_obs", decoded)

                if terminal:
                    break

            if decoded is None:
                decoded = {
                    "throughput": {s: 0.0 for s in SLICE_NAMES},
                    "latency": {s: 0.0 for s in SLICE_NAMES},
                }

            embb_thr_norm = float(decoded["throughput"].get("eMBB", 0.0))
            urllc_lat_norm = float(decoded["latency"].get("URLLC", 0.0))
            embb_thr = embb_thr_norm * float(cfg["env"]["max_thr_mbps"]["eMBB"])
            urllc_lat = urllc_lat_norm * float(cfg["env"]["max_lat_ms"]["URLLC"])
            sla_rate = compute_sla_rate(decoded, cfg)

            rec = {
                "episode": ep,
                "reward": round(ep_reward, 6),
                "steps": step_count,
                "embb_thr": round(embb_thr, 6),
                "urllc_lat": round(urllc_lat, 6),
                "sla_rate": round(sla_rate, 6),
            }
            logf.write(json.dumps(rec) + "\n")
            logf.flush()

            print(
                f"Episode {ep}/{args.episodes} | Reward: {ep_reward:.2f} | Steps: {step_count} | "
                f"eMBB: {embb_thr:.2f} Mbps | URLLC: {urllc_lat:.2f}ms | SLA: {sla_rate * 100:.1f}%"
            )

            if ep % save_every == 0:
                agent.save(model_dir / f"dqn_ep{ep}.pt")

    agent.save(model_dir / "dqn_final.pt")
    env.close()


if __name__ == "__main__":
    main()
