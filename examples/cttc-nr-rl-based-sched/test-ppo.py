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
    """!
    Proximal Policy Optimization (PPO) implementation for training a reinforcement learning agent.

    @param state_shape: The shape of the state space, typically a tuple representing the observation dimensions.
    @param action_dim: The dimension of the action space, representing the number of possible actions.
    @param hidden_dim: The size of the hidden layers for the neural network (default is 64).
    @param lr: Learning rate for the optimizer (default is 0.0003).
    @param gamma: Discount factor for future rewards (default is 0.99).
    @param eps_clip: Clipping value for the surrogate loss to stabilize training (default is 0.2).
    @param k_epochs: Number of epochs for which PPO updates are applied (default is 4).

    @return PPO object for policy optimization during the RL simulation.
    """

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
        """!
        A neural network module that contains both the actor and critic components for PPO.

        @param state_shape: Shape of the state space (e.g., for a 3D observation, it could be (batch_size, 3, 4)).
        @param action_dim: Number of possible actions the agent can take.
        @param hidden_dim: Dimension of hidden layers for both actor and critic networks.

        @return ActorCritic object with methods to compute actions and evaluate states.
        """

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
            """!
            Takes in the current state of the environment and outputs an action based on the policy.

            @param state: The state of the environment, usually a multi-dimensional array (e.g., (batch_size, 3, 4)).
            @return Tuple containing the selected action and the log-probability of that action.
            """
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
            """!
            Evaluates the state and action to compute log-probabilities, state values, and entropy.

            @param state: The state from the environment, potentially batch-processed.
            @param action: The action taken by the agent.
            @return Tuple containing the log-probabilities of the actions, the state values (critic), and the entropy.
            """
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
        """!
        Selects an action based on the current policy and stores relevant information in memory.

        @param state: The current state of the environment.
        @param memory: An instance of the Memory class to store states, actions, and log probabilities for future updates.
        @return action: The action selected by the policy.
        """
        action, action_logprob = self.policy_old.act(state)
        memory.states.append(state)
        memory.actions.append(action)
        memory.logprobs.append(action_logprob)
        return action

    def update(self, memory):
        """!
        Updates the policy using the memory of past actions and states based on the PPO update rule.

        @param memory: Memory instance containing past states, actions, log probabilities, and rewards.
        @return None: This function updates the policy parameters based on the experience stored in memory.
        """
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
    """!
    Memory class for storing states, actions, log probabilities, rewards, and terminal flags during training.

    The PPO algorithm requires storage of past experiences (states, actions, rewards, etc.) so that they can be
    used during the policy update step. This class implements a memory buffer using dequeues with a fixed size to
    prevent excessive memory consumption.

    @param maxlen: Maximum length of the memory buffer. The default value is 1000, which means the memory will hold
                   up to 1000 samples before older samples start being discarded.

    @return Memory object for storing experiences during the RL simulation.
    """

    def __init__(self, maxlen=1000):
        self.states = deque(maxlen=maxlen)
        self.actions = deque(maxlen=maxlen)
        self.logprobs = deque(maxlen=maxlen)
        self.rewards = deque(maxlen=maxlen)
        self.is_terminals = deque(maxlen=maxlen)

    def clear_memory(self):
        """!
        Clears the memory of stored states, actions, log probabilities, rewards, and terminal flags after an update.

        @return None: This function resets the memory buffers.
        """
        self.states.clear()
        self.actions.clear()
        self.logprobs.clear()
        self.rewards.clear()
        self.is_terminals.clear()


def debug(msg, debug_flag):
    """!
    Prints debug messages if the debug flag is enabled.

    @param msg: The debug message to print.
    @param debug_flag: A boolean flag indicating whether to print debug information.
    @return None: This function prints debug messages when necessary.
    """
    if debug_flag:
        print(msg)


def main(args):
    """!
    Main function for running the PPO-based reinforcement learning simulation using the ns-3 gym environment.

    @param args: Command-line arguments containing simulation parameters, such as the number of UEs,
                 simulation time, bandwidth, and other necessary configuration options.

    @return None: This function runs the PPO agent in the environment, selecting actions, and updating the model.
    """
    simArgs = {
        "--ueNum": args.ueNum,
        "--logging": args.logging,
        "--priorityTrafficScenario": args.priorityTrafficScenario,
        "--simTime": args.simTime,
        "--numerology": args.numerology,
        "--centralFrequency": args.centralFrequency,
        "--bandwidth": args.bandwidth,
        "--totalTxPower": args.totalTxPower,
        "--simTag": args.simTag,
        "--outputDir": args.outputDir,
        "--enableOfdma": args.enableOfdma,
        "--enableQoSLcScheduler": args.enableQoSLcScheduler,
        "--schedulerType": "Ai",  # Type of scheduler being used in the simulation (here, AI-based)
    }
    # Create the environment
    env = ns3env.Ns3Env(port=args.port, simSeed=args.simSeed, simArgs=simArgs, debug=args.debug)
    state_shape = env.observation_space.shape
    action_dim = env.action_space.shape[0]
    ppo = PPO(state_shape, action_dim)
    memory = Memory(maxlen=args.stepInterval)

    # Training the agent
    step_idx = 0
    step_interval = args.stepInterval
    try:
        obs = env.reset()
        while True:
            state = np.zeros(state_shape)
            state.flat[: obs.shape[0]] = obs
            action = ppo.select_action(state, memory)
            debug(f"State: {state.reshape(state_shape)}", args.debug)
            debug(f"Selected action: {action}", args.debug)

            obs, reward, done, _ = env.step(action)
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
    parser = argparse.ArgumentParser()
    # Arguments used for the gym script
    parser.add_argument("--port", type=int, default=5552, help="Port number")
    parser.add_argument("--simSeed", type=int, default=3002, help="Seed number")
    parser.add_argument("--debug", type=bool, default=False, help="Debug mode")
    # Arguments used for the ns3 simulation (simArgs)
    parser.add_argument(
        "--ueNum", type=int, default=2, help="Number of UEs (User Equipment) in the simulation"
    )
    parser.add_argument(
        "--logging",
        type=bool,
        default=False,
        help="Enable or disable logging during the simulation",
    )
    parser.add_argument(
        "--priorityTrafficScenario",
        type=int,
        default=0,
        help="The traffic scenario for the case of priority. Can be 0: saturation or 1: medium-load",
    )
    parser.add_argument("--simTime", type=int, default=1, help="Total simulation time in seconds")
    parser.add_argument(
        "--numerology", type=int, default=0, help="Numerology parameter for the simulation"
    )
    parser.add_argument(
        "--centralFrequency",
        type=float,
        default=4e9,
        help="Central frequency for the simulation in Hz",
    )
    parser.add_argument(
        "--bandwidth", type=float, default=10e6, help="Bandwidth in Hz for the simulation"
    )
    parser.add_argument(
        "--totalTxPower", type=float, default=43, help="Total transmission power in dBm"
    )
    parser.add_argument(
        "--simTag", type=str, default="default", help="Tag to label the simulation for reference"
    )
    parser.add_argument(
        "--outputDir", type=str, default="./", help="Directory where output data will be stored"
    )
    parser.add_argument(
        "--enableOfdma", type=int, default=0, help="Whether to enable OFDMA in the simulation"
    )
    parser.add_argument(
        "--enableQoSLcScheduler",
        type=int,
        default=0,
        help="Whether to enable QoS LC scheduler",
    )
    # Update interval for the PPO algorithm
    parser.add_argument(
        "--stepInterval", type=int, default=1000, help="Step interval for updating the PPO"
    )
    args = parser.parse_args()

    main(args)
