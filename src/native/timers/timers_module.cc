#include "native/timers/timers_module.h"

#include "runtime/instance.h"
#include "runtime/scheduler.h"

#include <js_engine.h>
#include <js_module.h>

const char* TimersPlugin::name() const {
    return "timers";
}

void TimersPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    (void)engine;
    auto& m = root.module("timers");

    m.funcDynamic("setTimeout", 2, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!JS_IsFunction(c, argv[0])) {
            return JS_ThrowTypeError(c, "setTimeout: callback must be function");
        }
        int64_t delay = 0;
        if (JS_ToInt64(c, &delay, argv[1]) < 0) {
            return JS_EXCEPTION;
        }

        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return JS_ThrowInternalError(c, "setTimeout: no active runtime instance");
        }

        int64_t id = 0;
        if (!inst->scheduler().add_timer(c, argv[0], delay, false, id)) {
            return JS_ThrowInternalError(c, "setTimeout: timer could not be scheduled");
        }
        return JS_NewInt64(c, id);
    });

    m.funcDynamic("setInterval", 2, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!JS_IsFunction(c, argv[0])) {
            return JS_ThrowTypeError(c, "setInterval: callback must be function");
        }
        int64_t delay = 0;
        if (JS_ToInt64(c, &delay, argv[1]) < 0) {
            return JS_EXCEPTION;
        }

        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return JS_ThrowInternalError(c, "setInterval: no active runtime instance");
        }

        int64_t id = 0;
        if (!inst->scheduler().add_timer(c, argv[0], delay, true, id)) {
            return JS_ThrowInternalError(c, "setInterval: timer could not be scheduled");
        }
        return JS_NewInt64(c, id);
    });

    m.func("clearTimeout", [](int64_t id) {
        if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
            inst->scheduler().cancel_timer(id);
        }
    });

    m.func("clearInterval", [](int64_t id) {
        if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
            inst->scheduler().cancel_timer(id);
        }
    });
}
