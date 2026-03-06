from pydantic import BaseModel, Field


class SliceTarget(BaseModel):
    throughput_mbps: float | None = None
    latency_ms: float | None = None
    devices: int | None = None


class ScenarioRequest(BaseModel):
    name: str = Field(..., min_length=3)
    duration_s: int = Field(default=20, ge=1, le=3600)
    agent: str = Field(default="dqn")
    slice_targets: dict[str, SliceTarget]


class RunResponse(BaseModel):
    status: str
    command: str
    stdout: str
    stderr: str
    return_code: int
