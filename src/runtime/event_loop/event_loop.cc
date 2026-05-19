#include "runtime/event_loop/event_loop.h"

#include "runtime/instance.h"
#include "runtime/scheduler.h"

#if QIANJS_HAVE_LIBUV
#include <uvw.hpp>
#endif

namespace qianjs::event_loop {

namespace {

Scheduler* active_scheduler() {
    RuntimeInstance* inst = RuntimeInstance::current();
    return inst ? &inst->scheduler() : nullptr;
}

} // namespace

#if QIANJS_HAVE_LIBUV

namespace uv {

std::shared_ptr<uvw::loop> uvw_loop() {
    Scheduler* s = active_scheduler();
    if (!s) {
        return nullptr;
    }
    return s->uvw_loop();
}

uv_loop_t* loop() {
    Scheduler* s = active_scheduler();
    if (!s) {
        return nullptr;
    }
    return s->raw_loop();
}

} // namespace uv

#endif

void ensure_started() {
    if (Scheduler* s = active_scheduler()) {
        s->ensure_started();
    }
}

void tick() {
    if (Scheduler* s = active_scheduler()) {
        s->tick();
    }
}

void defer(std::function<void(qjs::JSEngine&)> fn) {
    if (Scheduler* s = active_scheduler()) {
        s->defer(std::move(fn));
        return;
    }
}

void run_deferred(qjs::JSEngine& engine) {
    if (Scheduler* s = active_scheduler()) {
        s->run_deferred(engine);
    }
}

void begin_operation() {
    if (Scheduler* s = active_scheduler()) {
        s->begin_operation();
    }
}

void end_operation() {
    if (Scheduler* s = active_scheduler()) {
        s->end_operation();
    }
}

int pending_operations() {
    if (Scheduler* s = active_scheduler()) {
        return s->pending_operations();
    }
    return 0;
}

void shutdown() {
    if (RuntimeInstance* inst = RuntimeInstance::current()) {
        inst->scheduler().shutdown(inst->engine());
    }
}

} // namespace qianjs::event_loop
