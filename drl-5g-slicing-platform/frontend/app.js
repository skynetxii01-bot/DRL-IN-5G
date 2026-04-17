const api = "http://localhost:8000";
const scenarioEl = document.getElementById("scenario");
const outputEl = document.getElementById("output");

scenarioEl.value = JSON.stringify(
  {
    name: "three-slice-baseline",
    duration_s: 20,
    agent: "dqn",
    slice_targets: {
      embb: { throughput_mbps: 50, latency_ms: 20 },
      urllc: { throughput_mbps: 10, latency_ms: 5 },
      mmtc: { devices: 200 },
    },
  },
  null,
  2,
);

const log = (value) => {
  outputEl.textContent = typeof value === "string" ? value : JSON.stringify(value, null, 2);
};

document.getElementById("save").addEventListener("click", async () => {
  try {
    const payload = JSON.parse(scenarioEl.value);
    const res = await fetch(`${api}/scenario`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    log(await res.json());
  } catch (err) {
    log(`Scenario save error: ${err}`);
  }
});

document.getElementById("run").addEventListener("click", async () => {
  try {
    const res = await fetch(`${api}/run`, { method: "POST" });
    log(await res.json());
  } catch (err) {
    log(`Run error: ${err}`);
  }
});
