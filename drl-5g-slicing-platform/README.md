# AI-Powered Dynamic Resource Allocation in 5G NR Network Slicing (DRL + ns-3)

This project scaffold provides a **full-stack simulation workflow** for dynamic radio resource allocation across 5G NR network slices using **Deep Reinforcement Learning (DRL)** with:

- **ns-3.45 allinone**
- **5G-LENA v4.1.y**
- **OpenGym / ns3-gym integration**

It is designed as a practical starting point for experimentation, reproducibility, and extension.

---

## 1) Architecture

- **Simulation Layer (`sim/`)**
  - Wraps ns-3 execution commands
  - Stores scenario configuration (slice mix, numerology, QoS targets)
  - Placeholder training pipeline for DRL agent
- **Backend API (`backend/`)**
  - FastAPI service exposing APIs to load scenarios and launch runs
  - Handles execution orchestration and returns logs/status
- **Frontend (`frontend/`)**
  - Lightweight dashboard to configure scenario JSON and trigger runs
  - Displays run output and status

---

## 2) Recommended host setup

### Install ns-3.45 allinone

```bash
cd ~
wget https://www.nsnam.org/releases/ns-allinone-3.45.tar.bz2
tar -xjf ns-allinone-3.45.tar.bz2
cd ns-allinone-3.45
./build.py --enable-examples --enable-tests
```

### Add 5G-LENA v4.1.y

```bash
cd ~/ns-allinone-3.45/ns-3.45/contrib
git clone https://gitlab.com/cttc-lena/nr.git
cd nr
git checkout v4.1.y
```

### Add OpenGym

```bash
cd ~/ns-allinone-3.45/ns-3.45/contrib
git clone https://github.com/tkn-tub/ns3-gym.git
```

Reconfigure + build ns-3:

```bash
cd ~/ns-allinone-3.45/ns-3.45
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

> In this repo, set `NS3_ROOT` env var to your ns-3.45 folder path.

---

## 3) Run the full stack

From `drl-5g-slicing-platform/`:

```bash
docker compose up --build
```

Services:

- Frontend: http://localhost:8080
- Backend API: http://localhost:8000/docs

---

## 4) Local run without Docker

### Backend

```bash
cd backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --reload --port 8000
```

### Frontend

Open `frontend/index.html` in browser or serve via:

```bash
cd frontend
python3 -m http.server 8080
```

---

## 5) Simulation contract

Backend expects a scenario payload such as:

```json
{
  "name": "three-slice-baseline",
  "duration_s": 20,
  "agent": "dqn",
  "slice_targets": {
    "embb": {"throughput_mbps": 50, "latency_ms": 20},
    "urllc": {"throughput_mbps": 10, "latency_ms": 5},
    "mmtc": {"devices": 200}
  }
}
```

`sim/run_ns3.sh` is currently a wrapper script where you can connect your exact ns-3 example command (including 5G-LENA + OpenGym arguments).

---

## 6) Next implementation steps

1. Implement/attach ns-3 example (e.g., custom gNB scheduler with OpenGym hooks).
2. Define RL observation/action spaces in simulation C++ module.
3. Replace placeholder `train_dqn.py` with PPO/DQN production training loop.
4. Add metric persistence (SQLite/PostgreSQL) and experiment tracking.
5. Add KPI plots (slice throughput, delay CDF, PRB allocation fairness, reward trend).

---

## 7) Environment variables

- `NS3_ROOT`: absolute path to `~/ns-allinone-3.45/ns-3.45`
- `NS3_EXAMPLE`: ns-3 run target (default provided in `sim/run_ns3.sh`)
- `SIM_TIMEOUT_S`: simulation command timeout in seconds

