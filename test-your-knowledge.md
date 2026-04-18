# QA 🛰️
### Test your knowledge on this project

---

## What do you need to run in order to test?

```bash
git clone <repo>
mkdir build && cd build
cmake ..
make
./orion_tests
```

- `cmake ..` — reads `CMakeLists.txt` and generates a Makefile (a recipe book for the compiler)
- `make` — follows that recipe, calls the compiler, and produces a binary
- `./orion_tests` — actually executes that binary on the CPU

> **Key insight:** Three separate steps means three separate places a CI pipeline can fail — and each failure tells you something different.

---

## Why did we use `struct` for `TelemetryFrame` instead of `class`?

`struct` = plain data, `class` = behavior + encapsulation.

Using `struct` communicates to another engineer: *"this is just a data container, no hidden logic, no invariants to protect."* It's a deliberate signal about intent.

---

## In `CMakeLists.txt` we wrote `-Wall -Wextra`. What does the `-W` stand for and why do we care?

`-Wall` and `-Wextra` are **warnings**, not errors — yet. They tell the compiler: *"show me everything suspicious."* They don't stop compilation by themselves.

> **"A warning is a bug you haven't triggered yet. In flight software, warnings are unacceptable because the edge case that triggers them might be 200 miles above Earth. Treating warnings as errors means nothing ships with known issues."**

---

## What is `-Wpedantic` doing?

It means: *"enforce strict ISO C++ compliance, flag anything that's a compiler extension."*

If you write portable code that runs on multiple hardware targets, non-standard extensions are dangerous.

---

## Why are we using `uint8_t` in the `enum class SubsystemID` in `telemetry.hpp`?

`uint8_t` is 8 bits = 1 byte, which can hold values 0–255. We only have 5 subsystems (`POWER`, `THERMAL`, `ATTITUDE`, `COMMS`, `PROPULSION`) — we'll never need more than 255. Using a plain `int` would waste 4 bytes per frame for no reason.

Additionally, with `enum class` the compiler rejects anything that isn't a named subsystem. If you used a plain `int`, someone could pass `99` and the compiler wouldn't complain. Entire categories of bugs become impossible.

---

## Why do we use `hardware_valid` instead of simply checking the values?

Consider: a power sensor is physically damaged but still transmitting. It reports `0.0` volts. Is that a dead power bus or a broken sensor?

- `hardware_valid = false` → *"don't trust this reading at all"* — a **hardware fault**
- `voltage = 0.0` → *"the power bus just dropped to zero, sound the alarm"* — a **system fault**

Treating them the same could mean ignoring a real emergency or panicking over a bad sensor. That distinction is critical in safety systems.

---

## Why do we use `#pragma once` at the beginning of `.hpp` files?

It tells the compiler:

> *"No matter how many times this file gets included across the project, only process it once."*

Without it, if two different files both include the same header, the compiler would see duplicate definitions and throw an error.

---

## We split code into `include/` (headers) and `src/` (implementations). Why does that separation matter for a HIL test system?

The header in `include/` defines the **interface** — the *what*. The `.cpp` in `src/` defines the **implementation** — the *how*.

Because the test engine only depends on the interface, you can have two completely different implementations behind it:

| Context | File | Behavior |
|---|---|---|
| Testing | `simulator.cpp` | Generates fake frames with known values |
| Production | `hardware_driver.cpp` | Reads from a real sensor |

The test engine never changes. You swap implementations by changing one file — nothing else in the system needs to know.

> This concept has a name: **dependency injection**.

---

## In `TelemetryFrame`, the timestamp is `uint64_t`. Why not `int` or `float`?

**No negatives.** A timestamp in milliseconds since boot can never be negative. `uint` (unsigned) uses all 64 bits for positive numbers instead of wasting one bit on a sign — doubling the range.

**Float precision fails you.** A `float` only has ~7 decimal digits of precision. A satellite running for a few days has a timestamp in the hundreds of millions of milliseconds — a `float` starts rounding at that scale. You'd get two timestamps that are 1ms apart but look identical. In a test system, you can't reconstruct the order events happened in.

**`int` overflows.** A 32-bit `int` maxes out at ~2.1 billion. In milliseconds that's only ~24 days before it wraps back to zero. Timestamp overflow is a real, famous bug — GPS systems have hit this in production.

> **TLDR:** Timestamps are never negative, floats lose precision at large values, and 32-bit integers overflow in weeks. In a long-running satellite system, any of those bugs would corrupt your entire test timeline. `uint64_t` gives you ~585 million years of headroom.