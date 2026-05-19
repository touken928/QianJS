#pragma once

#include "platform/platform_window.h"
#include "runtime/instance.h"

#include <quickjs.h>

namespace qianjs::platform {

inline PlatformWindow* window_from_instance(qianjs::RuntimeInstance* inst) {
    return inst ? &inst->ensure_window() : nullptr;
}

inline PlatformWindow* require_window(JSContext* c) {
    qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
    PlatformWindow* win = window_from_instance(inst);
    if (!win || !win->inited()) {
        (void)JS_ThrowTypeError(c, "ui: call init() first");
        return nullptr;
    }
    return win;
}

} // namespace qianjs::platform
