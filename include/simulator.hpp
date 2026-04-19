#pragma once
#include "telemetry.hpp"

/**
 * This is a class instead of a struct because we want to add functions  to it (behaviour)
 */
class TelemetrySimulator {
    public: 
    // **Explicut keyword** - c++ must construct this intentionally and prevents c++ from converting SubsystemID into a TelemetrySimulator by mistake. 
        //Constructor- takes a subsystem and a nominal value to simulate telemetry data for that subsystem.
        //One simulator = one subsystem.
        explicit TelemetrySimulator(SubsystemID subsystem, float nominal_value);

        //Generates the next telemetry frame for this subsystem.
        //Every call advances in time and produces a reading.
        TelemetryFrame next();

        //Inject a fault - forces hardware_valid = false or N frames
        //To test that our anomaly detector catches it.
        void inject_fault(int duration_frames);

        //Inject a spike - pushes value way outside nominar for N frames
        //Hardware is still valid, but the value is bad.
        void inject_spike(float spike_value, int duration_frames);
    
        void reset(); //Resets simulator to clean state - useful between runs.

        
        //Encapsulation - we keep the internal state private and only interact with it through public functions.
    private:
        SubsystemID subsystem_; 
        float nominal_value_; //What normal looks like for this sensor
        float current_value_; // tracks drift over time
        uint64_t timestamp_ms_;  //advances with each frame 
        int fault_frames_; //countdown: how many fault frames left
        int spike_frames_; //countdown: how many spike frames left
        float spike_value_; //what value to report during a spike
};