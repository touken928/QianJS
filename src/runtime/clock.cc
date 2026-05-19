#include "runtime/clock.h"

#include <algorithm>

namespace qianjs {

void Clock::reset() {
    last_ = std::chrono::steady_clock::now();
    has_last_ = true;
    last_real_dt_ = 0;
    accumulator_ = 0;
    alpha_ = 0;
}

double Clock::tick() {
    const auto now = std::chrono::steady_clock::now();
    if (!has_last_) {
        reset();
        return 0;
    }
    last_real_dt_ = std::chrono::duration<double>(now - last_).count();
    last_ = now;

    if (fixed_dt_ > 0) {
        accumulator_ += std::min(last_real_dt_, 0.05);
        alpha_ = std::min(1.0, accumulator_ / fixed_dt_);
    }
    return last_real_dt_;
}

int Clock::consume_fixed_steps(double max_frame_dt) {
    if (fixed_dt_ <= 0) {
        return 0;
    }
    (void)max_frame_dt;
    int steps = 0;
    constexpr int kCap = 8;
    while (accumulator_ >= fixed_dt_ && steps < kCap) {
        accumulator_ -= fixed_dt_;
        ++steps;
    }
    alpha_ = fixed_dt_ > 0 ? std::min(1.0, accumulator_ / fixed_dt_) : 0;
    return steps;
}

} // namespace qianjs
