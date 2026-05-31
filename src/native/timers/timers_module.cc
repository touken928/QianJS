#include "native/timers/timers_module.h"

#include "runtime/instance.h"
#include "runtime/scheduler.h"

#include <qjs/call.h>
#include <qjs/module.h>

const char* TimersPlugin::name() const {
    return "timers";
}

void TimersPlugin::install(qjs::Context& ctx, qjs::Module& root) {
    (void)ctx;
    auto& m = root.module("timers");

    m.funcDynamic("setTimeout", 2, 2, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto cb = ctx.valueArg(0);
        if (!cb.success) {
            return cb;
        }
        if (!cb.value.isFunction()) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"setTimeout: callback must be function", {}, {}});
        }
        auto delay = ctx.int64Arg(1);
        if (!delay.success) {
            return qjs::Result<qjs::Value>::fail(delay.error);
        }

        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"setTimeout: no active runtime instance", {}, {}});
        }

        int64_t id = 0;
        if (!inst->scheduler().add_timer(std::move(cb.value), delay.value, false, id)) {
            return qjs::Result<qjs::Value>::ok(
                ctx.throwTypeError("setTimeout: timer could not be scheduled"));
        }
        return qjs::Result<qjs::Value>::ok(ctx.engine().int64(id));
    });

    m.funcDynamic("setInterval", 2, 2, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto cb = ctx.valueArg(0);
        if (!cb.success) {
            return cb;
        }
        if (!cb.value.isFunction()) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"setInterval: callback must be function", {}, {}});
        }
        auto delay = ctx.int64Arg(1);
        if (!delay.success) {
            return qjs::Result<qjs::Value>::fail(delay.error);
        }

        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"setInterval: no active runtime instance", {}, {}});
        }

        int64_t id = 0;
        if (!inst->scheduler().add_timer(std::move(cb.value), delay.value, true, id)) {
            return qjs::Result<qjs::Value>::ok(
                ctx.throwTypeError("setInterval: timer could not be scheduled"));
        }
        return qjs::Result<qjs::Value>::ok(ctx.engine().int64(id));
    });

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
