#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only

import argparse
import os
import time

import gym
import matplotlib.pyplot as plt
import numpy as np
from ns3gym import ns3env


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
    stepIdx = 0
    print("Start")
    # Run an episode
    try:
        obs = env.reset()
        print("Step: ", stepIdx)
        print("---obs: ", obs)

        while True:
            stepIdx += 1

            action = env.action_space.sample()
            obs, reward, done, info = env.step(action)

            if stepIdx % 1000 == 0:
                print("Step: ", stepIdx)
                print("---action: ", action)
                print("---obs, reward, done, info: ", obs, reward, done, info)

            if done:
                break

    except KeyboardInterrupt:
        print("Ctrl-C -> Exit")
    finally:
        env.close()
        print("Done")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=5552, help="Port number")
    parser.add_argument("--seed", type=int, default=3002, help="Seed number")
    parser.add_argument("--debug", type=bool, default=False, help="Debug mode")
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
    args = parser.parse_args()

    main(args)
