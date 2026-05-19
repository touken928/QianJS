#pragma once

#include "runtime/instance.h"

#include <js_engine.h>

namespace qianjs {

/** Thin facade over RuntimeInstance for native modules (promises, pump). */
class ScriptVm {
public:
    explicit ScriptVm(RuntimeInstance& instance) : instance_(instance) {}

    RuntimeInstance& instance() { return instance_; }
    qjs::JSEngine& engine() { return instance_.engine(); }
    const qjs::JSEngine& engine() const { return instance_.engine(); }

    qjs::JSEngine::PromiseHandle createPromise() { return instance_.create_promise(); }
    void freePromise(qjs::JSEngine::PromiseHandle h) { instance_.release_promise(h); }

    void pump_async() { instance_.pump_async(); }
    void pump_microtasks() { instance_.pump_microtasks(); }
    void pump_once() { instance_.pump_once(); }

private:
    RuntimeInstance& instance_;
};

} // namespace qianjs
