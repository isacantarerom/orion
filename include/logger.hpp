#pragma once
#include "telemetry.hpp"
#include "test_engine.hpp"
#include "anomaly_detector.hpp"
#include <string>
#include <vector>
#include <fstream>

class Logger {
public:

    // non modifyable: explicit
    // Constructor takes output file path so the logger owns the file
    //for its lifetime.
    explicit Logger(const std::string& filepath);

    // Destructor - closes the file
    //If the profram crashes, we want the file flushed and closed cleanly
    ~Logger();

    //Log a single telemetry frame with its test results
    void log_frame(
        const TelemetryFrame& frame,
        const std::vector<TestResult>& results,
        const AnomalyDetector& detector
    );

    //write the final summary at the end of a run
    void log_summary (
        const TestEngine& engine,
        const AnomalyDetector& detector
    );

    //Flush and close - call when done
    void close();

private:

    std::ofstream file_; //Actual file handle
    std::string filepath_;
    bool first_entry_; //Tracks if we need a comma before the next entry
                        //JSONS need commas between entreies but not after the last one

    //internal helpers - build JSON strins
    std::string escape(const std::string& s) const;
    std::string status_to_string(TestStatus s) const;
    std::string state_to_string(AnomalyState s) const;
};  