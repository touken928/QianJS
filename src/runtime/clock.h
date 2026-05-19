#pragma once

#include <chrono>

namespace qianjs {

/** Monotonic frame clock with optional fixed timestep accumulator. */
class Clock {
public:
    using time_point = std::chrono::steady_clock::time_point;

    void reset();
    double tick();

    void set_fixed_dt(double seconds) { fixed_dt_ = seconds > 0 ? seconds : 0; }
    double fixed_dt() const { return fixed_dt_; }
    double real_dt() const { return last_real_dt_; }
    double alpha() const { return alpha_; }

    /** Consume accumulator; returns number of fixed steps (capped). */
    int consume_fixed_steps(double max_frame_dt = 0.05);

private:
    time_point last_{};
    bool has_last_ = false;
    double last_real_dt_ = 0;
    double fixed_dt_ = 0;
    double accumulator_ = 0;
    double alpha_ = 0;
};

} // namespace qianjs
