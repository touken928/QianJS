#pragma once

namespace qianjs {

/** Explicit runtime phases; native code must respect these when touching JS. */
enum class LifecyclePhase {
    Created,
    Initialized,
    Running,
    Draining,
    Shutdown,
    Destroyed,
};

inline bool phase_allows_js(LifecyclePhase p) {
    return p == LifecyclePhase::Running || p == LifecyclePhase::Draining;
}

inline bool phase_allows_new_async(LifecyclePhase p) {
    return p == LifecyclePhase::Running;
}

inline bool phase_is_running(LifecyclePhase p) {
    return p == LifecyclePhase::Running;
}

} // namespace qianjs
