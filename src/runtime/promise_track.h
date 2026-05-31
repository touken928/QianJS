#pragma once

#include "runtime/instance.h"

#include <qjs/engine.h>
#include <memory>
#include <utility>

namespace qianjs::promise {

inline std::unique_ptr<qjs::Promise> create(qjs::Engine& engine) {
    auto p = engine.createPromise();
    if (RuntimeInstance* inst = RuntimeInstance::current()) {
        if (p) {
            inst->track_promise(p.get());
        }
    }
    return p;
}

inline void release(qjs::Promise* p) {
    if (RuntimeInstance* inst = RuntimeInstance::current()) {
        inst->untrack_promise(p);
    }
}

inline std::shared_ptr<qjs::Promise> create_shared(qjs::Engine& engine) {
    auto p = create(engine);
    if (!p) {
        return nullptr;
    }
    return std::shared_ptr<qjs::Promise>(std::move(p));
}

} // namespace qianjs::promise
