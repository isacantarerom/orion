#include "../include/anomaly_detector.hpp"

AnomalyDetector::AnomalyDetector(
    float min_nominal,
    float max_nominal,
    float consecutive_bad,
    float consecutive_good
) :
    min_nominal_(min_nominal),
    max_nominal_(max_nominal),
    consecutive_bad_threshold_(consecutive_bad),
    consecutive_good_threshold_(consecutive_good),
    state_(AnomalyState::NOMINAL),
    consecutive_bad_(0),
    consecutive_good_(0),
    anomaly_count_(0)
{ }

//A frame is only nominal if hardware is valid AND value is in range.
bool AnomalyDetector::is_nominal(const TelemetryFrame& frame) const {
    return frame.hardware_valid && frame.value >= min_nominal_ && frame.value <= max_nominal_;
}

bool AnomalyDetector::update(const TelemetryFrame& frame) {
    AnomalyState previous_state = state_;
    bool frame_nominal = is_nominal(frame);

    switch(state_) {
        case AnomalyState::NOMINAL:
            if(!frame_nominal) {
                consecutive_bad_++;
                consecutive_good_ = 0; // reset good counter streak
                
                if(consecutive_bad_ >= consecutive_bad_threshold_) {
                    transition(AnomalyState::ANOMALY,
                    "Sustained bad readings detected",
                    frame);
                } 
            } else {
                    consecutive_bad_ = 0; // One good frame resets the bad streak.
                }
        break;

        case AnomalyState::ANOMALY:
            if(frame_nominal) {
                consecutive_good_++;
                transition(AnomalyState::RECOVERING, "Readin improving after anomaly", frame);
            } else {
                consecutive_good_ = 0; // reset good counter streak
            }
        break;

        case AnomalyState::RECOVERING:
            if(frame_nominal) {
                consecutive_good_++;
                //Trust is harder to earn than lose, so we require twice as good frames to return to nominal.
                if(consecutive_good_ >= consecutive_good_threshold_ * 2) { 
                    transition(AnomalyState::NOMINAL, "Sustained recovery confirmed.", frame); 
                }
            } else {
                consecutive_good_ = 0; // reset good counter.
                transition(AnomalyState::ANOMALY, "Relased during recovery.", frame);
            }
        break;
    }

    return previous_state != state_; // Return true if a state transition just occurred.
}


void AnomalyDetector::transition(
    AnomalyState new_state,
    const std::string& reason,
    const TelemetryFrame& frame ) 
    {
    if(new_state == AnomalyState::ANOMALY) {
        anomaly_count_++;
    }

    events_.push_back({
        frame.timestamp_ms,
        state_, // from_state
        new_state,
        reason,
        frame.value
    });

    state_ = new_state;
    consecutive_bad_ = 0;
    consecutive_good_ = 0;

}

std::string AnomalyDetector::state_name() const {
    switch(state_) {
        case AnomalyState::NOMINAL: return "NOMINAL";
        case AnomalyState::ANOMALY: return "ANOMALY";
        case AnomalyState::RECOVERING: return "RECOVERING";
        default: return "UNKNOWN";
    }
}

void AnomalyDetector::reset() {
    state_ = AnomalyState::NOMINAL;
    consecutive_bad_ = 0;
    consecutive_good_ = 0;
    anomaly_count_ = 0;
    events_.clear();
}