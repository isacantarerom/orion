#include "../include/test_engine.hpp"

//We implement the TestEngine functions here. The header file just declares the interface, and this .cpp file defines the behavior.

//Caller gets a result immediately to react to it.
//Engine keeps a copy for final report.
//Two consumers, one operation.
TestResult TestEngine::record(TestResult result) {
    results_.push_back(result);
    return result;
}

TestResult TestEngine::check_limits(
    const std::string& test_name,
    const TelemetryFrame& frame,
    float min_expected,
    float max_expected ) 
{
    //Guard clause - hardware invalid is a different problem
    // We do not check on data we can't trust.
    if(!frame.hardware_valid) {
        return record(
            {
                test_name,
                TestStatus::ERROR,
                "Hardware reported invalid reading, cannot check limits.",
                frame.value,
                (min_expected + max_expected) / 2.0f //midpoint as "expected"
            }
        );
    }

    
    //The actual limit check:
    if(frame.value < min_expected || frame.value > max_expected) {
        return record ({
            test_name,
            TestStatus::FAIL,
            "Value was outside acceptable range.",
            frame.value,
            (min_expected + max_expected) / 2.0f //midpoint as "expected"
        });
    }

    //If we made it here, the test passed!
    return record ({
        test_name,
        TestStatus::PASS,
        "Value was within acceptable range.",
        frame.value,
        (min_expected + max_expected) / 2.0f //midpoint as "expected"
    });
}

TestResult TestEngine::check_hardware_valid (
    const std::string& test_name,
    const TelemetryFrame& frame
) {
    if(!frame.hardware_valid) {
        return record({
            test_name,
            TestStatus::ERROR,
            "Hardware validity check failed.",
            0.0f, // 0.0 = false, we expected hardware to be valid
            1.f // 1.0 = true, we expected hardware to be valid
        });
    }

    return record({
        test_name,
        TestStatus::PASS ,
        "Hardware validity check passed.",
        1.0f, // 1.0 = true, hardware is valid
        1.0f
    });
}

std::vector<TestResult> TestEngine::run_suite(
    const TelemetryFrame& frame,
    float min_expected,
    float max_expected) 
{
    std::vector<TestResult> suite_results;

    //Guard clause, always check hardware valitity first.
    suite_results.push_back(
        check_hardware_valid("Hardware_validity", frame)
    );

    suite_results.push_back(
        check_limits("value_in_range", frame, min_expected, max_expected)
);

    return suite_results;
}


int TestEngine::total_passed() const {
    int count = 0;

    for(const auto& result : results_) {
        if(result.status == TestStatus::PASS) count++;
    }
    return count;
}


int TestEngine::total_failed() const {
    int count = 0;

    for(const auto& result : results_) {
        if(result.status == TestStatus::FAIL) count++;
    }
    return count;
}

int TestEngine::total_errors() const {
    int count = 0;

    for(const auto& result : results_) {
        if(result.status == TestStatus::ERROR) count++;
    }
    return count;
}

void TestEngine::clear() {
    results_.clear();
}