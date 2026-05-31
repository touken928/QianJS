#include "runtime/plugin_lifecycle.h"

#include "runtime/instance.h"

namespace qianjs {

void notify_lifecycle(LifecyclePhase phase, RuntimeInstance& instance) {
    (void)phase;
    (void)instance;
}

} // namespace qianjs
