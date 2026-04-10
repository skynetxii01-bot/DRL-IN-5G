"""R2D2 agent with LSTM and prioritized replay."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np
import torch
import torch.nn.functional as F
from torch.nn.utils import clip_grad_norm_

from agents.r2d2.per_buffer import PrioritizedReplayBuffer, SequenceTransition
from agents.r2d2.r2d2_network import R2D2Network


@dataclass
class R2D2Config:
    obs_dim: int = 15
    action_dim: int = 27
    lr: float = 1e-4
    gamma: float = 0.99
    batch_size: int = 32
    seq_len: int = 40
    burn_in: int = 40
    n_step: int = 5
    per_alpha: float = 0.6
    per_beta_start: float = 0.4
    per_beta_end: float = 1.0
    buffer_size: int = 100000
    grad_clip: float = 40.0


class R2D2Agent:
    def __init__(self, cfg: R2D2Config, device: torch.device):
        self.cfg = cfg
        self.device = device
        self.online = R2D2Network(cfg.obs_dim, cfg.action_dim).to(device)
        self.target = R2D2Network(cfg.obs_dim, cfg.action_dim).to(device)
        self.target.load_state_dict(self.online.state_dict())
        self.optim = torch.optim.Adam(self.online.parameters(), lr=cfg.lr)
        self.buffer = PrioritizedReplayBuffer(cfg.buffer_size, alpha=cfg.per_alpha)
        self.train_steps = 0

    @torch.no_grad()
    def act(self, obs: np.ndarray, hidden=None, explore=True):
        obs_t = torch.tensor(obs, dtype=torch.float32, device=self.device).view(1, 1, -1)
        q, hidden_out = self.online(obs_t, hidden)
        q = q[0, -1]
        if explore and np.random.rand() < 0.05:
            a = int(np.random.randint(self.cfg.action_dim))
        else:
            a = int(torch.argmax(q).item())
        return a, hidden_out

    def add_sequence(self, obs, action, reward, done, next_obs, priority=1.0):
        self.buffer.add(
            SequenceTransition(obs=np.asarray(obs), action=np.asarray(action), reward=np.asarray(reward), done=np.asarray(done), next_obs=np.asarray(next_obs)),
            priority=priority,
        )

    def train_step(self):
        if len(self.buffer) < self.cfg.batch_size:
            return {"loss": 0.0}

        beta = min(1.0, self.cfg.per_beta_start + self.train_steps * 1e-6)
        idx, samples, weights = self.buffer.sample(self.cfg.batch_size, beta=beta)

        obs = torch.tensor(np.stack([s.obs for s in samples]), dtype=torch.float32, device=self.device)
        act = torch.tensor(np.stack([s.action for s in samples]), dtype=torch.long, device=self.device)
        rew = torch.tensor(np.stack([s.reward for s in samples]), dtype=torch.float32, device=self.device)
        done = torch.tensor(np.stack([s.done for s in samples]), dtype=torch.float32, device=self.device)
        nxt = torch.tensor(np.stack([s.next_obs for s in samples]), dtype=torch.float32, device=self.device)
        w = torch.tensor(weights, dtype=torch.float32, device=self.device).unsqueeze(1)

        q, _ = self.online(obs)
        q_taken = q.gather(-1, act.unsqueeze(-1)).squeeze(-1)

        with torch.no_grad():
            q_next_online, _ = self.online(nxt)
            next_actions = q_next_online.argmax(dim=-1, keepdim=True)
            q_next_target, _ = self.target(nxt)
            q_next = q_next_target.gather(-1, next_actions).squeeze(-1)
            target = rew + (1.0 - done) * self.cfg.gamma * q_next

        td = q_taken - target
        loss = (w * td.pow(2)).mean()

        self.optim.zero_grad(set_to_none=True)
        loss.backward()
        clip_grad_norm_(self.online.parameters(), self.cfg.grad_clip)
        self.optim.step()

        self.target.load_state_dict(self.online.state_dict())
        self.buffer.update_priorities(idx, np.abs(td.detach().mean(dim=1).cpu().numpy()) + 1e-6)
        self.train_steps += 1
        return {"loss": float(loss.item())}

    def save(self, path):
        torch.save({"online": self.online.state_dict(), "cfg": self.cfg.__dict__}, path)
