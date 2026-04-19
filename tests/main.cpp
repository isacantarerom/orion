#include <iostream>
#include "../include/telemetry.hpp"
#include "../include/simulator.hpp"

int main() {

    std::cout << "[ORION] - Telemetry Simulator \n";
    std::cout << "-------------------------\n\n";


    //Create a power subsystem simulator
    //Nominal value: 28.3 volts (typical for a satellite power bus)
    TelemetrySimulator power_sim(SubsystemID::POWER, 28.3f);

    // ----- Test 1: Normal operation -----
    std::cout << "[TEST] Normal telemetry stream (10 frames)";
    for(int i = 0; i < 10; i++){
        TelemetryFrame frame = power_sim.next();
        std::cout << "time = " << frame.timestamp_ms << "ms"
        << " value = " << frame.value << " volts" <<
        " hardware_valid = " << (frame.hardware_valid ? "Yes" : "No")
        << "\n";
    } 

    // ----- Test 2: Inject a fault -----
    std::cout << "\n [TEST] Injectin hardware fault for 3 frames: \n";
    power_sim.inject_fault(3);

    for(int i = 0; i < 5; i++) {
        TelemetryFrame frame = power_sim.next();
        std::cout << "time = " << frame.timestamp_ms << "ms"
        << " value = " << frame.value << " volts" 
        << " hardware_valid = " << (frame.hardware_valid ? "Yes" : "No")
        << "\n";
    }

    // ----- Test 3: Inject a spike -----
    std::cout << "\n Inject voltage spike (42.0V) for 2 frames: \n";
    power_sim.inject_spike(42.0f, 2);
    for(int i = 0; i < 5; i++) {
        TelemetryFrame frame = power_sim.next();
        std::cout << "time = " << frame.timestamp_ms << "ms"
        << " value = " << frame.value << " volts" 
        << " hardware_valid = " << (frame.hardware_valid ? "Yes" : "No")
        << "\n";
    }

    std::cout << "\n [ORION] Telemetry Simulator tests complete.\n";
    std::cout << "-------------------------\n\n";
    return 0;

    /*
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
    */
}

