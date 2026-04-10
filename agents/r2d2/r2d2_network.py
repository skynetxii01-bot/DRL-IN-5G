"""R2D2 dueling LSTM network."""

from __future__ import annotations

import torch
import torch.nn as nn


class R2D2Network(nn.Module):
    def __init__(self, obs_dim: int = 15, action_dim: int = 27, hidden_size: int = 256) -> None:
        super().__init__()
        self.encoder = nn.Sequential(
            nn.Linear(obs_dim, 128),
            nn.ReLU(),
            nn.Linear(128, 128),
            nn.ReLU(),
        )
        self.lstm = nn.LSTM(128, hidden_size, batch_first=True)
        self.value = nn.Sequential(nn.Linear(hidden_size, 128), nn.ReLU(), nn.Linear(128, 1))
        self.adv = nn.Sequential(nn.Linear(hidden_size, 128), nn.ReLU(), nn.Linear(128, action_dim))

    def forward(self, obs_seq: torch.Tensor, hidden=None):
        # obs_seq: [B, T, obs_dim]
        b, t, _ = obs_seq.shape
        x = self.encoder(obs_seq.reshape(b * t, -1)).reshape(b, t, -1)
        y, hidden_out = self.lstm(x, hidden)
        v = self.value(y)
        a = self.adv(y)
        q = v + a - a.mean(dim=-1, keepdim=True)
        return q, hidden_out
