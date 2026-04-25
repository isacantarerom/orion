# ORION 🛰️
### Orbital Runtime Integration & Observation Node

A Hardware-in-the-Loop (HIL) test framework written in C++, inspired by the kind of
test infrastructure used in real satellite software development.

Built as a learning project to deeply understand: test system design, anomaly detection,
CI pipelines, and debugging instincts for embedded/space systems.

---

## What it does

ORION simulates a stream of satellite telemetry (power, thermal, attitude, comms,
propulsion), runs a test engine against that data, detects anomalies, logs results,
and produces a structured report — all triggerable from a single CI-friendly script.

```
[Telemetry Simulator] → [Test Engine] → [Anomaly Detector] → [Logger] → [Python Reporter]
↑                                                                        ↑
(fake hardware)                                                        (human report)
```

---

## Project Structure

```
orion/
├── CMakeLists.txt          # Build system
├── run_tests.sh            # CI entry point — run this to do everything
├── include/                # Interfaces (.hpp) — the WHAT
│   ├── telemetry.hpp
│   ├── simulator.hpp
│   ├── test_engine.hpp
│   ├── anomaly_detector.hpp
│   └── logger.hpp
├── src/                    # Implementations (.cpp) — the HOW
│   ├── simulator.cpp
│   ├── test_engine.cpp
│   ├── anomaly_detector.cpp
│   └── logger.cpp
├── tests/
│   └── main.cpp            # Test entry point
└── reporter/
    └── report.py           # Python analysis layer
```

---

## How to build and run

```bash
# Build
mkdir build && cd build
cmake ..
make

# Run
./orion_tests

# Or run everything at once via CI script (wip)
./run_tests.sh
```

---

## Concepts covered

| Concept | Where |
|---|---|
| HIL test framework design | `simulator.hpp`, `test_engine.hpp` |
| Anomaly detection + state machines | `anomaly_detector.cpp` |
| Structured logging (JSON output) | `logger.cpp` |
| CI pipeline design | `run_tests.sh` + GitHub Actions |
| Data analysis + reporting | `reporter/report.py` |
| Dependency injection + testability | `simulator.hpp` interface pattern |

---

## Milestones

- [x] Day 1 — Scaffold + telemetry data model
- [x] Day 2 — Telemetry simulator (fake hardware)
- [x] Day 3 — Test engine + assertions
- [x] Day 4 — Anomaly detector
- [x] Day 5 — Logger + JSON output
- [x] Day 6 — Python reporter + Bash CI script
- [x] Day 7 — Chaos mode + interview dry run



[Test Your Knowledge QA](test-your-knowledge.md)