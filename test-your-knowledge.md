# QA 🛰️
### Test your knowledge on this project

---

### What do you need to run in order to test?

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

### Why did we use `struct` for `TelemetryFrame` instead of `class`?

`struct` = plain data, `class` = behavior + encapsulation.

Using `struct` communicates to another engineer: *"this is just a data container, no hidden logic, no invariants to protect."* It's a deliberate signal about intent.

---

### In `CMakeLists.txt` we wrote `-Wall -Wextra`. What does the `-W` stand for and why do we care?

`-Wall` and `-Wextra` are **warnings**, not errors — yet. They tell the compiler: *"show me everything suspicious."* They don't stop compilation by themselves.

> **"A warning is a bug you haven't triggered yet. In flight software, warnings are unacceptable because the edge case that triggers them might be 200 miles above Earth. Treating warnings as errors means nothing ships with known issues."**

---

### What is `-Wpedantic` doing?

It means: *"enforce strict ISO C++ compliance, flag anything that's a compiler extension."*

If you write portable code that runs on multiple hardware targets, non-standard extensions are dangerous.

---

### Why are we using `uint8_t` in the `enum class SubsystemID` in `telemetry.hpp`?

`uint8_t` is 8 bits = 1 byte, which can hold values 0–255. We only have 5 subsystems (`POWER`, `THERMAL`, `ATTITUDE`, `COMMS`, `PROPULSION`) — we'll never need more than 255. Using a plain `int` would waste 4 bytes per frame for no reason.

Additionally, with `enum class` the compiler rejects anything that isn't a named subsystem. If you used a plain `int`, someone could pass `99` and the compiler wouldn't complain. Entire categories of bugs become impossible.

---

### Why do we use `hardware_valid` instead of simply checking the values?

Consider: a power sensor is physically damaged but still transmitting. It reports `0.0` volts. Is that a dead power bus or a broken sensor?

- `hardware_valid = false` → *"don't trust this reading at all"* — a **hardware fault**
- `voltage = 0.0` → *"the power bus just dropped to zero, sound the alarm"* — a **system fault**

Treating them the same could mean ignoring a real emergency or panicking over a bad sensor. That distinction is critical in safety systems.

---

### Why do we use `#pragma once` at the beginning of `.hpp` files?

It tells the compiler:

> *"No matter how many times this file gets included across the project, only process it once."*

Without it, if two different files both include the same header, the compiler would see duplicate definitions and throw an error.

---

### We split code into `include/` (headers) and `src/` (implementations). Why does that separation matter for a HIL test system?

The header in `include/` defines the **interface** — the *what*. The `.cpp` in `src/` defines the **implementation** — the *how*.

Because the test engine only depends on the interface, you can have two completely different implementations behind it:

| Context | File | Behavior |
|---|---|---|
| Testing | `simulator.cpp` | Generates fake frames with known values |
| Production | `hardware_driver.cpp` | Reads from a real sensor |

The test engine never changes. You swap implementations by changing one file — nothing else in the system needs to know.

> This concept has a name: **dependency injection**.

---

### In `TelemetryFrame`, the timestamp is `uint64_t`. Why not `int` or `float`?

**No negatives.** A timestamp in milliseconds since boot can never be negative. `uint` (unsigned) uses all 64 bits for positive numbers instead of wasting one bit on a sign — doubling the range.

**Float precision fails you.** A `float` only has ~7 decimal digits of precision. A satellite running for a few days has a timestamp in the hundreds of millions of milliseconds — a `float` starts rounding at that scale. You'd get two timestamps that are 1ms apart but look identical. In a test system, you can't reconstruct the order events happened in.

**`int` overflows.** A 32-bit `int` maxes out at ~2.1 billion. In milliseconds that's only ~24 days before it wraps back to zero. Timestamp overflow is a real, famous bug — GPS systems have hit this in production.

> **TLDR:** Timestamps are never negative, floats lose precision at large values, and 32-bit integers overflow in weeks. In a long-running satellite system, any of those bugs would corrupt your entire test timeline. `uint64_t` gives you ~585 million years of headroom.

---

### Why does inject_fault set hardware_valid = false but inject_spike leaves it true?
"A fault means the hardware itself is broken — the sensor can't be trusted at all, so hardware_valid = false. A spike means the hardware is functioning correctly but reporting an abnormal value — maybe a real voltage surge happened. Those are two completely different failure modes that require completely different responses. You wouldn't replace a working sensor just because it detected a real spike."

---

### Why does next() use an early return when there's a fault?
"It's a guard clause — once we know the hardware is invalid, any value we'd compute is meaningless. There's no point running drift and noise calculations on data we're going to throw away. It also makes the code easier to read — the fault case is handled and exited cleanly, and everything below it can assume hardware is valid without needing nested if/else blocks."

---

### If you had to add a THERMAL simulator alongside the POWER one — what would you change?

"I'd just write TelemetrySimulator thermal_sim(SubsystemID::THERMAL, 45.0f) — 45 celsius being nominal for that sensor. The entire simulator logic — drift, noise, fault injection, spike injection — comes for free. I didn't write any thermal-specific code because the behavior is the same regardless of subsystem. That's reusability through good abstraction."


->> "what if THERMAL needs completely different behavior?" 
—"Then I'd create a subclass that inherits from a base simulator interface, overriding only what's different. The test engine would never need to change." That's Day 3 territory but good to have in your back pocket.

---

### What is a guard clause and why did we use one in next()? What would the code look like WITHOUT it?

A guard clause is when we return early since we don't want to compute things when we already know we don't need to. We protect the efforts.

> "Guard clauses eliminate nesting by handling exceptional cases first and exiting early. The happy path stays flat and easy to read."
---


### Look at the private section of TelemetrySimulator. Why is fault_frames_ private? What bad thing could happen if it was public?

"Private state means there's exactly one place where that state changes — the method I wrote. That makes bugs traceable. Public state can be modified from anywhere, which means bugs can come from anywhere."

---

### What is an initializer list in C++ and why did we use one in the constructor instead of assigning values inside the constructor body?

In out TelemetrySimulator constructor, that -> " : subsystem_(subsystem), nominal_value_(nominal_value)... " part — that's the initializer list. Everything between the : and the {}.

For simple types like float and int the difference is invisible — the compiler is smart enough that it doesn't matter. But for complex types like std::string or another class, here's what actually happens without an initializer list:

C++ default constructs the member first — creates an empty version
Then your assignment overwrites it with the real value

That's two operations. With an initializer list it's just one — the member is constructed directly with the right value, no empty version created first.

In embedded and real-time systems, you care about every unnecessary operation. A class with 10 members being constructed twice per frame at 10 frames per second adds up. 

---

### What does the `explicit` keyword on our constructor do, and what bug does it prevent?

"explicit prevents the compiler from making automatic conversions you didn't ask for. In safety-critical systems, silent automatic behavior is dangerous — you want the compiler to force you to be intentional."

---

### What is the difference between `DRIFT_RATE` and `NOISE_AMPLITUDE` in the simulator? Why do we need both — what different real-world phenomena are they modelling?

|.     | Drift | Noise |
|------|-------|-------|
|Pattern | Smooth,redictable, follows a curve |Random, different every frame| 
|Real cause | Sensor warming up, aging, calibration shift| Electrical interference, vibration |
Detectable? | Yes — you can model and compensate for it | Hard — it's genuinely random |


> "Drift is the slow predictable wander of a sensor over time. Noise is the random jitter on every single reading. Real sensors have both simultaneously, which is why we model both."
---

### In one sentence — what is the difference between a `.hpp` file and a `.cpp` file? Explain it like you're talking to someone who has never coded before.

"The .hpp is the menu — it tells you what exists and what you can order. The .cpp is the kitchen — it's where the actual work happens."

---

### Look at this line in simulator.cpp: float noise = ((float)std::rand() / RAND_MAX - 0.5f) * NOISE_AMPLITUDE; — break it down. What is each part doing and WHY?


> "Without subtracting 0.5, all noise would be additive — the sensor would always read slightly high. Real noise is symmetric around zero, so we center the range."

---

###  At the top of simulator.cpp we wrote static const float DRIFT_RATE = 0.02f. What does static mean in this context and what does const mean? What breaks if you remove either one?

const — nobody can change this value at runtime.
static — this constant is invisible outside this file. It's hiding it from the rest of the project, not sharing it.

"static at file scope means 'private to this translation unit.' It prevents name collisions across files. const means the value never changes. Together they make a safe, local, immutable constant."

---

### When main.cpp calls power_sim.next() — walk me through what physically happens in memory and execution. Where does the TelemetryFrame live? Who owns it?

1. The program jumps to the next() function in memory
2. TelemetryFrame frame is created on the stack — a temporary chunk of memory that only exists inside next()
3. All the fields get filled in
4. return frame — here's the interesting part. The frame gets copied back to the caller. So main.cpp's TelemetryFrame frame = power_sim.next() has its own separate copy
5. The original frame inside next() is destroyed — it was on the stack, so it vanishes automatically when the function returns

---

### We have fault_frames_ and spike_frames_ as separate counters. What would go wrong if we used a single special_frames_ counter for both faults AND spikes?


"Separate counters let fault and spike states be completely independent. They can overlap, they can have different durations, and the code that handles each is unambiguous. Merging them loses information and makes simultaneous failure testing impossible."

---

### Our simulator uses std::sin() for drift. Why a sine wave specifically? What would be different if we used rand() for drift too instead of sin()?

The whole point of modeling them separately is that they represent different physical phenomena with different solutions:

Drift is smooth and predictable → you can compensate for it mathematically. If you know a sensor drifts by a sine wave pattern, you can subtract that pattern out of your readings.
Noise is random → you can't predict it, but you can filter it by averaging multiple readings.


> "sin() models drift as smooth and periodic because real sensor drift follows physical patterns — thermal expansion, aging — that are gradual and somewhat predictable. Random drift would be indistinguishable from noise, which defeats the purpose of modeling them as separate phenomena with different correction strategies."

---

### Why does reset() exist on the simulator? When would you actually call it in a real test suite and why is it critical for test independence?

"reset() exists to guarantee test independence — each test starts from identical known state regardless of what previous tests did. Without it, tests can contaminate each other and you get failures that only appear in certain run orders, which are the hardest bugs to debug."