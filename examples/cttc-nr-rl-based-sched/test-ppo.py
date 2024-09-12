#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Seoul National University (SNU)
# Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only

import argparse
from collections import deque

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from ns3gym import ns3env
from torch.distributions import Categorical


class PPO:
    def __init__(
        self,
        state_shape,
        action_dim,
        hidden_dim=64,
        lr=0.0003,
        gamma=0.99,
        eps_clip=0.2,
        k_epochs=4,
    ):
        self.gamma = gamma
        self.eps_clip = eps_clip
        self.k_epochs = k_epochs

        self.policy = self.ActorCritic(state_shape, action_dim, hidden_dim)
        self.optimizer = optim.Adam(self.policy.parameters(), lr=lr)
        self.policy_old = self.ActorCritic(state_shape, action_dim, hidden_dim)
        self.policy_old.load_state_dict(self.policy.state_dict())

        self.mse_loss = nn.MSELoss()

    class ActorCritic(nn.Module):
        def __init__(self, state_shape, action_dim, hidden_dim):
            super(PPO.ActorCritic, self).__init__()
            self.actor = nn.Sequential(
                nn.Linear(state_shape[1], hidden_dim),  # Expect input of shape (4,)
                nn.ReLU(),
                nn.Linear(hidden_dim, action_dim),
                nn.Softmax(dim=-1),
            )

            self.critic = nn.Sequential(
                nn.Linear(
                    state_shape[0] * state_shape[1], hidden_dim
                ),  # Input size is 12 after flattening
                nn.ReLU(),
                nn.Linear(hidden_dim, 1),
            )

        def forward(self):
            raise NotImplementedError

        def act(self, state):
            debug(
                f"State shape: {state.shape}", args.debug
            )  # Debugging: print the shape of the state
            state = torch.from_numpy(state).float()  # state is (1000, 3, 4)
            action_probs = []
            for i in range(state.shape[0]):  # Iterate over the batch
                debug(
                    f"State shape: {state[i].shape}", args.debug
                )  # Debugging: print the shape of each state
                action_prob = self.actor(state[i])  # state[i] is (3, 4)
                action_probs.append(action_prob)

            action_probs = torch.stack(
                action_probs
            )  # Stack results into a (1000, action_dim) tensor
            dist = Categorical(action_probs)
            actions = dist.sample()
            return actions.numpy(), dist.log_prob(actions)

        def evaluate(self, state, action):
            action_probs = []
            for i in range(state.shape[0]):  # state.shape[0] is 1000 (batch size)
                debug(
                    f"State shape: {state[i].shape}", args.debug
                )  # Debugging: print the shape of each state
                action_prob = self.actor(
                    state[i]
                )  # state[i] is (3, 4) for each element in the batch
                action_probs.append(action_prob)

            action_probs = torch.stack(
                action_probs
            )  # Stack results into a (1000, action_dim) tensor
            dist = Categorical(action_probs)

            action_log_probs = dist.log_prob(action)
            dist_entropy = dist.entropy()

            # Flatten each state in the batch for the critic
            flattened_state = state.view(state.shape[0], -1)  # Flatten each sample in the batch
            debug(
                f"Flattened state shape: {flattened_state.shape}", args.debug
            )  # Debugging: should print (1000, 12)

            state_value = self.critic(flattened_state)  # Pass flattened states to the critic

            return action_log_probs, torch.squeeze(state_value), dist_entropy

    def select_action(self, state, memory):
        action, action_logprob = self.policy_old.act(state)
        memory.states.append(state)
        memory.actions.append(action)
        memory.logprobs.append(action_logprob)
        return action

    def update(self, memory):
        debug("Memory update", args.debug)
        rewards = []
        discounted_reward = 0
        for reward, is_terminal in zip(reversed(memory.rewards), reversed(memory.is_terminals)):
            if is_terminal:
                discounted_reward = 0
            discounted_reward = reward + (self.gamma * discounted_reward)
            rewards.insert(0, discounted_reward)

        rewards = torch.tensor(rewards, dtype=torch.float32)
        rewards = (rewards - rewards.mean()) / (rewards.std() + 1e-5)

        old_states = torch.tensor(np.array(memory.states), dtype=torch.float32)
        old_actions = torch.tensor(np.array(memory.actions), dtype=torch.int64)

        # Convert logprobs more efficiently to avoid warnings
        if isinstance(memory.logprobs[0], torch.Tensor):
            old_logprobs = torch.stack(
                [lp.detach() for lp in memory.logprobs]
            )  # Stack tensors directly
        else:
            old_logprobs = torch.tensor(np.array(memory.logprobs), dtype=torch.float32)

        for _ in range(self.k_epochs):
            logprobs, state_values, dist_entropy = self.policy.evaluate(old_states, old_actions)

            ratios = torch.exp(logprobs - old_logprobs.detach())

            # Ensure advantages is of shape [1000, 3] to match ratios
            advantages = rewards - state_values.detach()
            advantages = advantages.unsqueeze(1)  # Now shape [1000, 1]

            surr1 = ratios * advantages  # Broadcasting should now work correctly
            surr2 = torch.clamp(ratios, 1 - self.eps_clip, 1 + self.eps_clip) * advantages

            loss = (
                -torch.min(surr1, surr2)
                + 0.5 * self.mse_loss(state_values, rewards)
                - 0.01 * dist_entropy
            )

            self.optimizer.zero_grad()
            loss.mean().backward()
            self.optimizer.step()

        self.policy_old.load_state_dict(self.policy.state_dict())


class Memory:
    def __init__(self, maxlen=1000):
        self.states = deque(maxlen=maxlen)
        self.actions = deque(maxlen=maxlen)
        self.logprobs = deque(maxlen=maxlen)
        self.rewards = deque(maxlen=maxlen)
        self.is_terminals = deque(maxlen=maxlen)

    def clear_memory(self):
        self.states.clear()
        self.actions.clear()
        self.logprobs.clear()
        self.rewards.clear()
        self.is_terminals.clear()


def debug(msg, debug_flag):
    if debug_flag:
        print(msg)


def main(args):
    simArgs = {
        "--ueNum": args.ueNum,
        "--logging": args.logging,
        "--priorityTrafficScenario": args.priorityTrafficScenario,
        "--numTrafficProfile": args.numTrafficProfile,
        "--simTime": args.simTime,
        "--numerology": args.numerology,
        "--centralFrequency": args.centralFrequency,
        "--bandwidth": args.bandwidth,
        "--totalTxPower": args.totalTxPower,
        "--simTag": args.simTag,
        "--outputDir": args.outputDir,
        "--enableOfdma": args.enableOfdma,
        "--enableAi": True,
    }
    # Create the environment
    env = ns3env.Ns3Env(port=args.port, simSeed=args.seed, simArgs=simArgs, debug=args.debug)
    state_shape = env.observation_space.shape
    action_dim = env.action_space.shape[0]
    ppo = PPO(state_shape, action_dim)
    memory = Memory(maxlen=args.stepInterval)

    # Training the agent
    step_idx = 0
    step_interval = args.stepInterval
    try:
        state = env.reset()
        while True:
            action = ppo.select_action(state.reshape(state_shape), memory)
            debug(f"State: {state.reshape(state_shape)}", args.debug)
            debug(f"Selected action: {action}", args.debug)

            state, reward, done, _ = env.step(action)
            debug(f"Reward: {reward}", args.debug)

            memory.rewards.append(reward)
            memory.is_terminals.append(done)

            if done:
                break

            if step_idx > 0 and step_idx % step_interval == 0:
                ppo.update(memory)
                memory.clear_memory()

            step_idx += 1

    except KeyboardInterrupt:
        print("Ctrl-C -> Exit")
    finally:
        env.close()
        print("Done")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    # Arguments for the environment
    parser.add_argument("--port", type=int, default=5552, help="Port number")
    parser.add_argument("--seed", type=int, default=3002, help="Seed number")
    parser.add_argument("--debug", type=bool, default=False, help="Debug mode")
    # Arguments for the simulation
    parser.add_argument("--ueNum", type=int, default=3, help="Number of UEs")
    parser.add_argument("--logging", type=bool, default=False, help="Logging")
    parser.add_argument(
        "--priorityTrafficScenario",
        type=int,
        default=0,
        help="The traffic scenario for the case of priority. Can be 0: saturation or 1: medium-load",
    )
    parser.add_argument(
        "--numTrafficProfile", type=int, default=3, help="Number of traffic profiles"
    )
    parser.add_argument("--simTime", type=int, default=1, help="Simulation time")
    parser.add_argument("--numerology", type=int, default=0, help="Numerology")
    parser.add_argument("--centralFrequency", type=float, default=4e9, help="Central frequency")
    parser.add_argument("--bandwidth", type=float, default=10e6, help="Bandwidth")
    parser.add_argument("--totalTxPower", type=float, default=43, help="Total Tx power")
    parser.add_argument("--simTag", type=str, default="default", help="Simulation tag")
    parser.add_argument("--outputDir", type=str, default="./", help="Output directory")
    parser.add_argument("--enableOfdma", type=bool, default=False, help="Enable OFDMA")
    # Update interval for the PPO algorithm
    parser.add_argument(
        "--stepInterval", type=int, default=1000, help="Step interval for updating the PPO"
    )
    args = parser.parse_args()

    main(args)
