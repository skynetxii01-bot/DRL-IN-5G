"""PPO agent with clipped objective and GAE."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np
import torch
import torch.nn.functional as F
from torch.utils.data import DataLoader, TensorDataset

from agents.ppo.actor_critic import ActorCritic
from agents.ppo.rollout_buffer import RolloutBuffer


@dataclass
class PpoConfig:
    obs_dim: int = 15
    action_dim: int = 27
    lr: float = 3e-4
    gamma: float = 0.99
    lambda_gae: float = 0.95
    eps_clip: float = 0.2
    n_steps: int = 2048
    epochs: int = 4
    batch_size: int = 64
    ent_coef: float = 0.01
    vf_coef: float = 0.5


class PpoAgent:
    def __init__(self, cfg: PpoConfig, device: torch.device) -> None:
        self.cfg = cfg
        self.device = device
        self.net = ActorCritic(cfg.obs_dim, cfg.action_dim).to(device)
        self.optim = torch.optim.Adam(self.net.parameters(), lr=cfg.lr)
        self.buffer = RolloutBuffer()

    @torch.no_grad()
    def act(self, obs: np.ndarray):
        obs_t = torch.tensor(obs, dtype=torch.float32, device=self.device).unsqueeze(0)
        action, log_prob, value = self.net.act(obs_t)
        return int(action.item()), float(log_prob.item()), float(value.item())

    def update(self, last_value: float = 0.0):
        if len(self.buffer.steps) == 0:
            return {"loss": 0.0}

        adv, ret = self.buffer.compute_gae(last_value, self.cfg.gamma, self.cfg.lambda_gae)
        adv = (adv - adv.mean()) / (adv.std() + 1e-8)

        obs = torch.tensor(np.stack([s.obs for s in self.buffer.steps]), dtype=torch.float32, device=self.device)
        actions = torch.tensor([s.action for s in self.buffer.steps], dtype=torch.long, device=self.device)
        old_log_probs = torch.tensor([s.log_prob for s in self.buffer.steps], dtype=torch.float32, device=self.device)
        returns = torch.tensor(ret, dtype=torch.float32, device=self.device)
        advantages = torch.tensor(adv, dtype=torch.float32, device=self.device)

        ds = TensorDataset(obs, actions, old_log_probs, returns, advantages)
        loader = DataLoader(ds, batch_size=self.cfg.batch_size, shuffle=True)

        total_loss = 0.0
        for _ in range(self.cfg.epochs):
            for b_obs, b_actions, b_old_logp, b_ret, b_adv in loader:
                dist, values = self.net.get_dist_value(b_obs)
                logp = dist.log_prob(b_actions)
                ratio = torch.exp(logp - b_old_logp)

                s1 = ratio * b_adv
                s2 = torch.clamp(ratio, 1 - self.cfg.eps_clip, 1 + self.cfg.eps_clip) * b_adv
                policy_loss = -torch.min(s1, s2).mean()
                value_loss = F.mse_loss(values, b_ret)
                entropy = dist.entropy().mean()

                loss = policy_loss + self.cfg.vf_coef * value_loss - self.cfg.ent_coef * entropy
                self.optim.zero_grad(set_to_none=True)
                loss.backward()
                self.optim.step()
                total_loss += float(loss.item())

        self.buffer.clear()
        return {"loss": total_loss}

    def save(self, path):
        torch.save({"model": self.net.state_dict(), "cfg": self.cfg.__dict__}, path)
