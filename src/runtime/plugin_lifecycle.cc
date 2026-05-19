#include "runtime/plugin_lifecycle.h"

#include "runtime/instance.h"

namespace qianjs {

void notify_lifecycle(LifecyclePhase phase, RuntimeInstance& instance) {
    if (phase != LifecyclePhase::Shutdown) {
        return;
    }
#if QIANJS_MODULE_UI
    instance.destroy_window();
#else
    (void)instance;
#endif
}

} // namespace qianjs
