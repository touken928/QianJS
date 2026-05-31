#pragma once

#include "platform/platform_window.h"
#include "runtime/instance.h"

#include <qjs/call.h>

namespace qianjs::platform {

inline PlatformWindow* window_from_instance(qianjs::RuntimeInstance* inst) {
    return inst ? &inst->ensure_window() : nullptr;
}

inline PlatformWindow* require_window(qjs::CallContext& ctx) {
    qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
    PlatformWindow* win = window_from_instance(inst);
    if (!win || !win->inited()) {
        (void)ctx.throwTypeError("ui: call init() first");
        return nullptr;
    }
    return win;
}

} // namespace qianjs::platform
