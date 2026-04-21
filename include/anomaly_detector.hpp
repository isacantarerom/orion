#pragma once
#include "telemetry.hpp"
#include <string>
#include <vector>

enum class AnomalyState {
    NOMINAL, //Everything is normal
    ANOMALY, //sustained problem detected
    RECOVERING // improving but not yet trusted
};


//A record of when a state transition happened.
struct AnomalyEvent {
    uint64_t timestamp_ms;
    AnomalyState from_state;
    AnomalyState to_state;
    std::string reason; // human readable - why did we transition.
    float value; // what value triggered it?
};

class AnomalyDetector {

public:
    //Constructor - thresholds define what "bad" means
    AnomalyDetector(
        float min_nominal,
        float max_nominal,
        float consecutive_bad = 3, //How many bad frmaes before declaring ANOMALY
        float consecutive_good = 3 // How many good frames before declaring NOMINAL
    );

    //Feed one frame into the detector
    //Returns true if a state transition just occurred
    bool update(const TelemetryFrame& frame); 

    //Current state
    AnomalyState state() const {return state_;}

    //Human readable state name = useful for logging
    std::string state_name() const;

    //Full history of every transition that occurred
    const std::vector<AnomalyEvent>& events() const {return events_;}

    //How anomalies have been detected total
    int anomaly_count() const {return anomaly_count_;}

    void reset();


private:

    float min_nominal_;
    float max_nominal_;
    int consecutive_bad_threshold_;
    int consecutive_good_threshold_;


    AnomalyState state_;
    int anomaly_count_;
    int consecutive_bad_;
    int consecutive_good_;

    std::vector<AnomalyEvent> events_;

    //Internal transition - records the event and chanfes state.
    void transition(AnomalyState new_state, const std::string& reason, const TelemetryFrame& frame);


    //Is this frame's value within nomal range?
    bool is_nominal(const TelemetryFrame& frame) const;
};