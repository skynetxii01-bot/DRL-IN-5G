"""PPO actor-critic network for 15-dim obs and 27 discrete actions."""

from __future__ import annotations

import torch
import torch.nn as nn
from torch.distributions import Categorical


class ActorCritic(nn.Module):
    def __init__(self, obs_dim: int = 15, action_dim: int = 27) -> None:
        super().__init__()
        self.shared = nn.Sequential(
            nn.Linear(obs_dim, 256),
            nn.LayerNorm(256),
            nn.ReLU(),
            nn.Linear(256, 256),
            nn.LayerNorm(256),
            nn.ReLU(),
        )
        self.actor = nn.Sequential(nn.Linear(256, 128), nn.ReLU(), nn.Linear(128, action_dim))
        self.critic = nn.Sequential(nn.Linear(256, 128), nn.ReLU(), nn.Linear(128, 1))

    def get_dist_value(self, obs: torch.Tensor):
        feat = self.shared(obs)
        logits = self.actor(feat)
        value = self.critic(feat).squeeze(-1)
        return Categorical(logits=logits), value

    def act(self, obs: torch.Tensor):
        dist, value = self.get_dist_value(obs)
        action = dist.sample()
        log_prob = dist.log_prob(action)
        return action, log_prob, value
