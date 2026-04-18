#include <iostream>
#include "../include/telemetry.hpp"

int main() {
    //Let's construct a telemetry frame simulating what hardware would send.
    TelemetryFrame frame;
    frame.subsystem = SubsystemID::POWER;
    frame.timestamp_ms = 1000;
    frame.value = 28.3f;
    frame.hardware_valid = true;


    std::cout << "[ORION] Book OK. First Telemetry frame loaded .\n";
    std::cout << "Subsystem : POWER\n";
    std::cout << "Value: " << frame.value << " volts\n";
    std::cout << "Hardware Valid: " << (frame.hardware_valid ? "Yes" : "No") << "\n";

    return 0;
}

