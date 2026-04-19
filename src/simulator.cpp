#include "./../include/simulator.hpp"
#include <cmath> // for sin() and fabs()
#include <cstdlib> // for rand()

//We define constants that are self-documenting at the top for easy tuning and readability.
//Magic numbers buried in code are undebuggable.
static const uint64_t FRAME_INTERVAL_MS = 100; // Simulate 10frames per second
static const float DRIFT_RATE = 0.02f; //How much value drifts per frame
static const float NOISE_AMPLITUDE = 0.1f; //Random noise range


TelemetrySimulator::TelemetrySimulator(SubsystemID subsystem, float nominal_value) :
    subsystem_(subsystem), //This is an initializer list - more efficient than assigning in the constructor body.
    nominal_value_(nominal_value),
    current_value_(nominal_value), // Start at nominal
    timestamp_ms_(0),
    fault_frames_(0),
    spike_frames_(0),
    spike_value_(0.0f)
{
    //Why an initializer list instead of assigning in the body?
    //Because it directly initializes the member variables, which can be more efficient than default constructing them
    //and then assigning new values in the constructor body. For simple types like float and int, the performance difference is negligible,
    //but for more complex types, it can save unnecessary construction and assignment operations.
}

TelemetryFrame TelemetrySimulator::next() {
    TelemetryFrame frame;
    frame.subsystem = subsystem_;
    frame.timestamp_ms = timestamp_ms_;

    //Advacnce time - every call moves the clock forward
    timestamp_ms_ += FRAME_INTERVAL_MS;

    //Handle fault injections first
    if(fault_frames_ > 0) {
        frame.hardware_valid = false;
        frame.value = 0.0f; //Value is garbage when hardware is invalid
        fault_frames_--; // Decrease fault capacity we have left
        return frame; // early return since we don't care about value if hardware is invalid
    }

    //Hardware is valid at this point
    frame.hardware_valid = true;

    //Handle spike injections
    if(spike_frames_ > 0) {
        frame.value = spike_value_;
        spike_frames_--; // Decrease spike capacity we have left
        return frame; // early return since we want to maintain the spike value during the spike duration
    }

    //Simulate normal telemetry behavior with drift and noise
    //Drift: value slowly wonders from nominal over time like a real sensonr warming up.
    //sin() creates a smooth oscillation rather than a random walk, which is more realistic for many sensors.

    float drift = std::sin(timestamp_ms_ * 0.001f) * DRIFT_RATE * nominal_value_;   // Drift oscillates over time
    
    //Noise: small random validation every frame
    //rand() returns a value between 0 and RAND_MAX, so we scale it to be between -NOISE_AMPLITUDE and +NOISE_AMPLITUDE
    float noise = ((float)std::rand() / RAND_MAX - 0.5f) * NOISE_AMPLITUDE; // Random noise in range [-NOISE_AMPLITUDE, +NOISE_AMPLITUDE]

    current_value_ = nominal_value_ + drift + noise;
    frame.value = current_value_;

    return frame;
}


void TelemetrySimulator::inject_fault(int duration_frames) {
    fault_frames_ = duration_frames;
}

void TelemetrySimulator::inject_spike(float spike_value, int duration_frames) {
    spike_value_ = spike_value;
    spike_frames_ = duration_frames;
}

void TelemetrySimulator::reset() {
    current_value_ = nominal_value_;
    timestamp_ms_ = 0;
    fault_frames_ = 0;
    spike_frames_ = 0;
    spike_value_ = 0.0f;
}
