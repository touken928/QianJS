#pragma once

#include "runtime/instance.h"

#include <js_engine.h>

namespace qianjs::promise {

inline qjs::JSEngine::PromiseHandle create(qjs::JSEngine& engine) {
    if (RuntimeInstance* inst = RuntimeInstance::current()) {
        return inst->create_promise();
    }
    return engine.createPromise();
}

inline void release(qjs::JSEngine& engine, qjs::JSEngine::PromiseHandle h) {
    if (RuntimeInstance* inst = RuntimeInstance::current()) {
        inst->release_promise(h);
        return;
    }
    engine.freePromise(h);
}

} // namespace qianjs::promise
