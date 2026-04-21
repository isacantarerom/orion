#pragma once
#include "telemetry.hpp"
#include <string>
#include <vector>


//Enum for the results.
//Strongly typed, self documenting, and prevents bugs related to using raw integers or strings for status codes.
enum class TestStatus {
    PASS,
    FAIL, //Value out of range
    ERROR //Hardware invalid
};


//One test result - everything we need to know about a single check
struct TestResult {
    std::string name; //What are we testing?
    TestStatus status;
    std::string message; //Why did it pass / fail / error?
    float actual; //What value did we actually see
    float expected; //What value did we expect to see?
};


//The engine itself: 

class TestEngine {
    public:
        //Check that a frame's value is within expected limits.
        TestResult check_limits(
            const std::string& test_name,
            const TelemetryFrame& frame,
            float min_expected,
            float max_expected
        );

        //Check that hardware is valid.
        TestResult check_hardware_valid(
            const std::string& test_name,
            const TelemetryFrame& frame
        );

        //Run a full suite of checks on a frame
        std::vector<TestResult> run_suite(
            const TelemetryFrame& frame,
            float min_expected,
            float max_expected
        );

        //Summary helpers
        int total_run() const { return results_.size(); }
        int total_passed() const;
        int total_failed() const;
        int total_errors() const;

        //Get all accumulated results
        const std::vector<TestResult>& results() const { return results_;}

        //Clear results between test runs
        void clear();


    private:

    std::vector<TestResult> results_; //Accumulates results across multiple checks and frames. Useful for reporting and analysis.

    //Internal helper - records a result and return it.
    //Private because nobody outside needs to record results directly.
    //They would go through check_limits, check_hardware_valid, or run_suite which will call record() for them.
    TestResult record(TestResult result);
};