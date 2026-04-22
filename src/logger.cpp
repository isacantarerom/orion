#include "logger.hpp"
#include <iostream>
#include <sstream>  // for std::ostringstream

Logger::Logger(const std::string& filepath)
    : filepath_(filepath)
    , first_entry_(true)
{
    file_.open(filepath);

    if (!file_.is_open()) {
        // Why std::cerr instead of std::cout?
        // cerr is for errors — it's unbuffered and always flushes immediately.
        // cout is buffered for performance. For error messages you never
        // want buffering — you want it on screen NOW.
        std::cerr << "[LOGGER] ERROR: could not open file: " << filepath << "\n";
        return;
    }

    // Start the JSON structure
    // We write a JSON object with two keys: "frames" and "summary"
    file_ << "{\n";
    file_ << "  \"frames\": [\n";
}

Logger::~Logger() {
    close();
}

void Logger::log_frame(
    const TelemetryFrame&          frame,
    const std::vector<TestResult>& results,
    const AnomalyDetector&         detector)
{
    if (!file_.is_open()) return;

    // JSON arrays need commas between entries — not before the first,
    // not after the last. first_entry_ tracks this.
    if (!first_entry_) {
        file_ << ",\n";
    }
    first_entry_ = false;

    file_ << "    {\n";
    file_ << "      \"timestamp_ms\": " << frame.timestamp_ms << ",\n";
    file_ << "      \"subsystem\": "    << (int)frame.subsystem << ",\n";
    file_ << "      \"value\": "        << frame.value << ",\n";
    file_ << "      \"hardware_valid\": " << (frame.hardware_valid ? "true" : "false") << ",\n";
    file_ << "      \"detector_state\": \"" << state_to_string(detector.state()) << "\",\n";

    // Nest the test results as an array inside each frame
    file_ << "      \"results\": [\n";
    for (size_t i = 0; i < results.size(); i++) {
        const auto& r = results[i];
        file_ << "        {\n";
        file_ << "          \"name\": \""    << escape(r.name)    << "\",\n";
        file_ << "          \"status\": \""  << status_to_string(r.status) << "\",\n";
        file_ << "          \"actual\": "    << r.actual   << ",\n";
        file_ << "          \"expected\": "  << r.expected << ",\n";
        file_ << "          \"message\": \"" << escape(r.message) << "\"\n";
        file_ << "        }";
        if (i < results.size() - 1) file_ << ",";
        file_ << "\n";
    }
    file_ << "      ]\n";
    file_ << "    }";
}

void Logger::log_summary(
    const TestEngine&      engine,
    const AnomalyDetector& detector)
{
    if (!file_.is_open()) return;

    // Close the frames array, open summary object
    file_ << "\n  ],\n";
    file_ << "  \"summary\": {\n";
    file_ << "    \"total_run\": "     << engine.total_run()    << ",\n";
    file_ << "    \"passed\": "        << engine.total_passed() << ",\n";
    file_ << "    \"failed\": "        << engine.total_failed() << ",\n";
    file_ << "    \"errors\": "        << engine.total_errors() << ",\n";
    file_ << "    \"anomaly_count\": " << detector.anomaly_count() << ",\n";

    // Log all state transition events
    file_ << "    \"anomaly_events\": [\n";
    const auto& events = detector.events();
    for (size_t i = 0; i < events.size(); i++) {
        const auto& e = events[i];
        file_ << "      {\n";
        file_ << "        \"timestamp_ms\": " << e.timestamp_ms << ",\n";
        file_ << "        \"from\": \""  << state_to_string(e.from_state) << "\",\n";
        file_ << "        \"to\": \""    << state_to_string(e.to_state)   << "\",\n";
        file_ << "        \"reason\": \"" << escape(e.reason) << "\",\n";
        file_ << "        \"value\": "   << e.value << "\n";
        file_ << "      }";
        if (i < events.size() - 1) file_ << ",";
        file_ << "\n";
    }
    file_ << "    ]\n";
    file_ << "  }\n";
    file_ << "}\n";
}

void Logger::close() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

std::string Logger::escape(const std::string& s) const {
    // Replace characters that would break JSON strings
    // Why does this matter? If a message contains a quote or backslash,
    // it would terminate the JSON string early and corrupt the file.
    std::string result;
    for (char c : s) {
        if      (c == '"')  result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else                result += c;
    }
    return result;
}

std::string Logger::status_to_string(TestStatus s) const {
    switch (s) {
        case TestStatus::PASS:  return "PASS";
        case TestStatus::FAIL:  return "FAIL";
        case TestStatus::ERROR: return "ERROR";
        default:                return "UNKNOWN";
    }
}

std::string Logger::state_to_string(AnomalyState s) const {
    switch (s) {
        case AnomalyState::NOMINAL:    return "NOMINAL";
        case AnomalyState::ANOMALY:    return "ANOMALY";
        case AnomalyState::RECOVERING: return "RECOVERING";
        default:                        return "UNKNOWN";
    }
}