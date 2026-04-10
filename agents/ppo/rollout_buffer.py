"""Rollout buffer with GAE utilities for PPO."""

from __future__ import annotations

from dataclasses import dataclass
from typing import List

import numpy as np


@dataclass
class RolloutStep:
    obs: np.ndarray
    action: int
    reward: float
    done: bool
    log_prob: float
    value: float


class RolloutBuffer:
    def __init__(self) -> None:
        self.steps: List[RolloutStep] = []

    def clear(self) -> None:
        self.steps.clear()

    def add(self, obs, action, reward, done, log_prob, value) -> None:
        self.steps.append(
            RolloutStep(
                obs=np.asarray(obs, dtype=np.float32),
                action=int(action),
                reward=float(reward),
                done=bool(done),
                log_prob=float(log_prob),
                value=float(value),
            )
        )

    def compute_gae(self, last_value: float, gamma: float, gae_lambda: float):
        n = len(self.steps)
        adv = np.zeros(n, dtype=np.float32)
        ret = np.zeros(n, dtype=np.float32)
        last_gae = 0.0
        for t in reversed(range(n)):
            next_value = last_value if t == n - 1 else self.steps[t + 1].value
            next_non_terminal = 0.0 if self.steps[t].done else 1.0
            delta = self.steps[t].reward + gamma * next_value * next_non_terminal - self.steps[t].value
            last_gae = delta + gamma * gae_lambda * next_non_terminal * last_gae
            adv[t] = last_gae
            ret[t] = adv[t] + self.steps[t].value
        return adv, ret
