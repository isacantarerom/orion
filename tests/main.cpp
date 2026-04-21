#include <iostream>
#include "../include/telemetry.hpp"
#include "../include/simulator.hpp"
#include "../include/test_engine.hpp"

void print_result(const TestResult& r) {
    std::string status_str;
    if(r.status == TestStatus::PASS) status_str = "PASS";
    else if(r.status == TestStatus::FAIL) status_str = "FAIL";
    else status_str = "ERROR";

    std::cout << "[" << status_str << "] " << r.name
    << " | actual: " << r.actual <<
    " | " << r.message << "\n"; 
}

int main() {

    std::cout << "[ORION] - Telemetry Simulator \n";
    std::cout << "-------------------------\n\n";

    TelemetrySimulator power_sim(SubsystemID::POWER, 28.3f);
    TestEngine engine;

    //Power bus nominal limits: 26V - 30V
    const float MIN_VOLTS = 26.0f;
    const float MAX_VOLTS = 30.0f;

    // ----- Suite 1: Normal operation -----
    std::cout << "[SUITE 1] Normal Operation (5 frames): \n";
    for(int i = 0; i < 5; i++) {
        auto frame = power_sim.next();
        auto results = engine.run_suite(frame, MIN_VOLTS, MAX_VOLTS);
        for(const auto& result : results) print_result(result);
    }

    //---- Suite 2: Hardware fault injection -----
    std::cout << "\n [SUITE 2] Hardware fault injected: \n";
    power_sim.inject_fault(3);
    for(int i = 0; i < 3; i++) {
        auto frame = power_sim.next();
        auto results = engine.run_suite(frame, MIN_VOLTS, MAX_VOLTS);
        for(const auto& result : results) print_result(result);
    }

    //---- Suite 3: Voltage Spike injection -----
    std::cout << "\n [SUITE 3] Voltage spike injected (42V): \n";
    power_sim.inject_spike(42.0f, 2);
    for(int i = 0; i < 2; i++) {
        auto frame = power_sim.next();
        auto results = engine.run_suite(frame, MIN_VOLTS, MAX_VOLTS);
        for(const auto& result : results) print_result(result);
    }

    // ---Final summary---
    std::cout << "\n [SUMMARY] \n";
    std::cout << "Total tests run: " << engine.total_run() << "\n";
    std::cout << "Total passed: " << engine.total_passed() << "\n";
    std::cout << "Total failed: " << engine.total_failed() << "\n";
    std::cout << "Total errors: " << engine.total_errors() << "\n";

    //If there are any fails or errors, we return 1 meaning failure.
    //That is how the CI system can mark it as a red build. (green for success, meaning no failures nore errors).
    //To check it on the terminal go: echo "Exist code: $?"
    // That would print 1 or 0 depending on whether we had a failure or not.
    return (engine.total_failed() + engine.total_errors()) > 0 ? 1 : 0;

    /*
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
    */

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

