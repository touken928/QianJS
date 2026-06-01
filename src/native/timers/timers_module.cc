#include "native/timers/timers_module.h"

#include "runtime/instance.h"
#include "runtime/scheduler.h"

#include <qjs/engine.h>
#include <qjs/module.h>
#include <qjs/value.h>

#include <functional>

const char* TimersPlugin::name() const {
    return "timers";
}

void TimersPlugin::install(qjs::Context& ctx, qjs::Module& root) {
    qjs::Engine& eng = ctx.engine();
    auto& m = root.module("timers");

    m.func("setTimeout", std::function<qjs::Value(qjs::Value, int64_t)>([&eng](qjs::Value cb, int64_t delay) -> qjs::Value {
        if (!cb.isFunction()) {
            return eng.throwTypeError("setTimeout: callback must be function");
        }
        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return eng.throwTypeError("setTimeout: no active runtime instance");
        }
        int64_t id = 0;
        if (!inst->scheduler().add_timer(std::move(cb), delay, false, id)) {
            return eng.throwTypeError("setTimeout: timer could not be scheduled");
        }
        return eng.int64(id);
    }));

    m.func("setInterval", std::function<qjs::Value(qjs::Value, int64_t)>([&eng](qjs::Value cb, int64_t delay) -> qjs::Value {
        if (!cb.isFunction()) {
            return eng.throwTypeError("setInterval: callback must be function");
        }
        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return eng.throwTypeError("setInterval: no active runtime instance");
        }
        int64_t id = 0;
        if (!inst->scheduler().add_timer(std::move(cb), delay, true, id)) {
            return eng.throwTypeError("setInterval: timer could not be scheduled");
        }
        return eng.int64(id);
    }));

    m.func("clearTimeout", std::function<void(int64_t)>([](int64_t id) {
        if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
            inst->scheduler().cancel_timer(id);
        }
    }));

    m.func("clearInterval", std::function<void(int64_t)>([](int64_t id) {
        if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
            inst->scheduler().cancel_timer(id);
        }
    }));
}
