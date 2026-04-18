#pragma once
#include <string>
#include <cstdint>

//We will use an enum to prvent entire categories of bugs.
enum class SubsystemID : uint8_t {
    POWER = 0,
    THERMAL = 1,
    ATTITUDE = 2, // orientation controll (pitch/roll/yaw)
    COMMS = 3,
    PROPULSION = 4
};


//One packet of data from a subsystem.
//"What a satellite sends down every second."
//->> We separate hardware valid because a bad reading from a sensor is different from a bad value.
struct TelemetryFrame {
    SubsystemID subsystem;
    uint64_t timestamp_ms; //milliseconds since boot
    float value; //the value of the telemetry data. Could be temperature, voltage, etc.
    bool hardware_valid; //whether the hardware is functioning properly. If false, the value may be garbage.
};

struct TelemetryLimits {
    float min_nominal;
    float max_nominal;
    float min_critical; //below this is critical = fault
    float max_critical; //above this is critical = fault
};


/**
 * 
 * If a sensor is giving us weird readings / bad data
 *  - We first want to know if the sensor is broken (hardware_valid = false)
 *  Then we want to know if the value is out of the nominal range (min_nominal, max_nominal)
 *  or critical range (min_critical, max_critical)
 *  Then we can decide how to respond to the anomaly (e.g. switch to backup sensor, enter safe mode, etc.)
 */