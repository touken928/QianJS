#pragma once

#include "runtime/lifecycle.h"

namespace qianjs {

class RuntimeInstance;

/** Invoked from RuntimeInstance on lifecycle transitions (e.g. destroy SDL on Shutdown). */
void notify_lifecycle(LifecyclePhase phase, RuntimeInstance& instance);

} // namespace qianjs
