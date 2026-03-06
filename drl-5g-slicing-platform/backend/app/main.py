import json
import os
import subprocess
from pathlib import Path

from fastapi import FastAPI, HTTPException

from .schemas import RunResponse, ScenarioRequest

app = FastAPI(title="DRL 5G Slicing Control API", version="0.1.0")

BASE_DIR = Path(__file__).resolve().parents[2]
SIM_DIR = BASE_DIR / "sim"
SCENARIO_PATH = SIM_DIR / "scenarios" / "active_scenario.json"
RUN_SCRIPT = SIM_DIR / "run_ns3.sh"


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.post("/scenario")
def set_scenario(payload: ScenarioRequest) -> dict[str, str]:
    SCENARIO_PATH.parent.mkdir(parents=True, exist_ok=True)
    SCENARIO_PATH.write_text(json.dumps(payload.model_dump(), indent=2), encoding="utf-8")
    return {"status": "saved", "path": str(SCENARIO_PATH)}


@app.post("/run", response_model=RunResponse)
def run_simulation() -> RunResponse:
    if not RUN_SCRIPT.exists():
        raise HTTPException(status_code=500, detail=f"Missing run script: {RUN_SCRIPT}")

    timeout_s = int(os.getenv("SIM_TIMEOUT_S", "300"))

    try:
        result = subprocess.run(
            ["bash", str(RUN_SCRIPT)],
            cwd=str(SIM_DIR),
            text=True,
            capture_output=True,
            timeout=timeout_s,
            check=False,
        )
    except subprocess.TimeoutExpired as exc:
        raise HTTPException(status_code=504, detail=f"Simulation timed out after {timeout_s}s") from exc

    return RunResponse(
        status="success" if result.returncode == 0 else "failed",
        command=f"bash {RUN_SCRIPT}",
        stdout=result.stdout,
        stderr=result.stderr,
        return_code=result.returncode,
    )
