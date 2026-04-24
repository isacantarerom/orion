#include <iostream>
#include "telemetry.hpp"
#include "simulator.hpp"
#include "test_engine.hpp"
#include "anomaly_detector.hpp"
#include "logger.hpp"

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

int main() {
    std::cout << "[ORION] Day 6 — Full Pipeline\n";
    std::cout << "==============================\n\n";

    TelemetrySimulator power_sim(SubsystemID::POWER, 28.3f);
    TestEngine         engine;
    AnomalyDetector    detector(26.0f, 30.0f, 3, 3);
    Logger             logger("logs/run.json");

    const float MIN_VOLTS = 26.0f;
    const float MAX_VOLTS = 30.0f;

    // --- Phase 1: Normal operation ---
    std::cout << "[PHASE 1] Normal operation (10 frames):\n";
    for (int i = 0; i < 10; i++) {
        auto frame       = power_sim.next();
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
    power_sim.inject_spike(42.0f, 6);
    for (int i = 0; i < 8; i++) {
        auto frame        = power_sim.next();
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
        auto frame        = power_sim.next();
        auto results      = engine.run_suite(frame, MIN_VOLTS, MAX_VOLTS);
        bool transitioned = detector.update(frame);
        logger.log_frame(frame, results, detector);

        if (transitioned) {
            std::cout << "  *** STATE CHANGE → " << detector.state_name() << "\n";
        }
    }
    std::cout << "  Detector state: " << detector.state_name() << "\n";

    // --- Anomaly event timeline ---
    print_events(detector);

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