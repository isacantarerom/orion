#include <iostream>
#include "telemetry.hpp"
#include "simulator.hpp"
#include "test_engine.hpp"
#include "anomaly_detector.hpp"
#include "logger.hpp"
#include <cstdlib>
#include <ctime>
#include <random>

    const float MIN_VOLTS = 26.0f;
    const float MAX_VOLTS = 30.0f;

void print_result(const TestResult& r) {
    std::string status_str;
    if      (r.status == TestStatus::PASS)  status_str = "PASS ";
    else if (r.status == TestStatus::FAIL)  status_str = "FAIL ";
    else                                     status_str = "ERROR";

    std::cout << "  [" << status_str << "] "
              << r.name
              << " | actual=" << r.actual
              << " | " << r.message << "\n";
}

void print_events(const AnomalyDetector& detector) {
    std::cout << "\n[ANOMALY EVENTS]\n";
    if (detector.events().empty()) {
        std::cout << "  No state transitions occurred.\n";
        return;
    }
    for (const auto& e : detector.events()) {
        std::cout << "  Time: " << e.timestamp_ms << "ms"
                  << " | " << e.reason
                  << " | Value: " << e.value << "\n";
    }
}


std::string print_subsystem(SubsystemID& sys) {
    switch (sys) 
    {
        case SubsystemID::POWER : return "POWER"; 
        case SubsystemID::THERMAL : return "THERMAL"; 
        case SubsystemID::ATTITUDE : return "ATTITUDE"; 
        case SubsystemID::COMMS : return "COMMS"; 
        case SubsystemID::PROPULSION : return "PROPULSION"; 
        
        default: return "POWER";
    }
}

float generate_volts(float min, float max) {
    std::random_device rd;
        std::mt19937 gen(rd());

    std::uniform_real_distribution<float>
        dist(min, max);
    
    return dist(gen);
}


void on_nomal_operation(TelemetrySimulator& simulator, TestEngine& engine, AnomalyDetector& detector, Logger& logger, int frames) {
    // --- Phase 1: Normal operation ---
    std::cout << "==============================\n\n";
    std::cout << "[PHASE 1] Normal operation: " << frames << "frames. " << "\n";
    for (int i = 0; i < frames; i++) {
        auto frame       = simulator.next();
        auto results     = engine.run_suite(frame, MIN_VOLTS, MAX_VOLTS);
        bool transitioned = detector.update(frame);
        logger.log_frame(frame, results, detector);

        if (transitioned) {
            std::cout << "  *** STATE CHANGE → " << detector.state_name() << "\n";
        }
    }
    std::cout << "  Detector state: " << detector.state_name() << "\n";

    // --- Phase 2: Sustained voltage spike ---
    std::cout << "\n[PHASE 2] Sustained voltage spike (42V, 6 frames):\n";
    simulator.inject_spike(42.0f, 6);
    for (int i = 0; i < 8; i++) {
        auto frame        = simulator.next();
        auto results      = engine.run_suite(frame, MIN_VOLTS, MAX_VOLTS);
        bool transitioned = detector.update(frame);
        logger.log_frame(frame, results, detector);

        if (transitioned) {
            std::cout << "  *** STATE CHANGE → " << detector.state_name() << "\n";
        }
        std::cout << "  t=" << frame.timestamp_ms
                  << "ms  value=" << frame.value
                  << "V  state=" << detector.state_name() << "\n";
    }

    // --- Phase 3: Recovery ---
    std::cout << "\n[PHASE 3] Recovery (10 frames of normal):\n";
    for (int i = 0; i < 10; i++) {
        auto frame        = simulator.next();
        auto results      = engine.run_suite(frame, MIN_VOLTS, MAX_VOLTS);
        bool transitioned = detector.update(frame);
        logger.log_frame(frame, results, detector);

        if (transitioned) {
            std::cout << "  *** STATE CHANGE → " << detector.state_name() << "\n";
        }
    }
    std::cout << "==============================\n\n";

    
}

void on_spike_operation(TelemetrySimulator& simulator, TestEngine& engine, AnomalyDetector& detector, Logger& logger, int spike_frame) {
    std::cout << "==============================\n\n";
    std::cout << "[SPIKE OPERATION]  " << spike_frame << "frames. " << "\n";
    for (int i = 0; i < spike_frame; i++) {
        float volts = generate_volts(MAX_VOLTS + 1.0, 100.0);
        simulator.inject_spike(volts, 1);
        auto frame        = simulator.next();
        auto results      = engine.run_suite(frame, MIN_VOLTS, MAX_VOLTS);
        bool transitioned = detector.update(frame);
        logger.log_frame(frame, results, detector);

        if (transitioned) {
            std::cout << "  *** STATE CHANGE → " << detector.state_name() << "\n";
        }
        std::cout << "  t=" << frame.timestamp_ms
                  << "ms  value=" << frame.value
                  << "V  state=" << detector.state_name() << "\n";
    }
    std::cout << "==============================\n\n";
}

void on_fault_operation(TelemetrySimulator& simulator, TestEngine& engine, AnomalyDetector& detector, Logger& logger, int fault_frame) {
    std::cout << "==============================\n\n";
    std::cout << "[FAULT OPERATION]  " << fault_frame << "frames. " << "\n";
    simulator.inject_fault(fault_frame);
    for(int i = 0; i < fault_frame; i++) {
        auto frame = simulator.next();
        auto results = engine.run_suite(frame, MIN_VOLTS, MAX_VOLTS);
        bool transitioned = detector.update(frame);
        logger.log_frame(frame, results, detector);

        if (transitioned) {
            std::cout << "  *** STATE CHANGE → " << detector.state_name() << "\n";
        }
        std::cout << "  t=" << frame.timestamp_ms
                  << "ms  value=" << frame.value
                  << "V  state=" << detector.state_name() << "\n";
    }
    std::cout << "==============================\n\n";
}




int main() {

    SubsystemID subsystem = SubsystemID::POWER;
    float nominal_volts = generate_volts(MIN_VOLTS, MAX_VOLTS);

    bool chaos_mode = (std::getenv("ORION_CHAOS") != nullptr);
    srand(time(nullptr));

    std::cout << "[ORION] Chaos mode: " << (chaos_mode ? "ENABLED" : "disabled") << "\n";
    int r = rand() % 5;
    subsystem = static_cast<SubsystemID>(r);
    std::cout << " ** CREATING SUBSYSTEM : " << print_subsystem(subsystem) << "  WITH " << nominal_volts << " AS NOMINAL VOLTS.";

    TelemetrySimulator simulator(subsystem, nominal_volts);
    TestEngine         engine;
    AnomalyDetector    detector(26.0f, 30.0f, 3, 3);
    const char* log_path = std::getenv("ORION_LOG");

    if (!log_path) {
        std::cerr << "[ERROR] Environment variable ORION_LOG not set. "
                << "Did you run via run_tests.sh?\n";
        return 1;  // exit cleanly instead of segfaulting
    }
    
    Logger logger(log_path ? log_path : "logs/run.json");

    if(chaos_mode) {
       bool is_fault = rand() % 2 == 0;
           std::cout << "[ORION] Chaos \n";
           std::cout << "==============================\n\n";
       
       if(is_fault) {
            on_fault_operation(simulator, engine, detector, logger, 10);
       } else {
            on_spike_operation(simulator, engine, detector, logger, 10);  
       }
    } else {
        on_nomal_operation(simulator, engine, detector, logger, 10);
    }

// --- Anomaly event timeline ---
    print_events(detector);
    std::cout << "  Detector state: " << detector.state_name() << "\n";

    // --- Write log and close ---
    logger.log_summary(engine, detector);
    logger.close();
    std::cout << "\n[LOG] Results written to logs/run.json\n";

    // --- Final summary ---
    std::cout << "\n[SUMMARY]\n";
    std::cout << "  Total tests run : " << engine.total_run()    << "\n";
    std::cout << "  Passed          : " << engine.total_passed() << "\n";
    std::cout << "  Failed          : " << engine.total_failed() << "\n";
    std::cout << "  Errors          : " << engine.total_errors() << "\n";
    std::cout << "  Anomalies found : " << detector.anomaly_count() << "\n";

    return (engine.total_failed() + engine.total_errors()) > 0 ? 1 : 0;
}