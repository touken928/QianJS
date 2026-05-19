#include "runtime/promise_registry.h"

namespace qianjs {

void PromiseRegistry::track(qjs::JSEngine::PromiseHandle h) {
    if (h.ptr) {
        pending_.insert(h.ptr);
    }
}

void PromiseRegistry::untrack(qjs::JSEngine::PromiseHandle h) {
    if (h.ptr) {
        pending_.erase(h.ptr);
    }
}

void PromiseRegistry::reject_all(qjs::JSEngine& engine, const char* message, const char* code) {
    const std::vector<void*> snapshot(pending_.begin(), pending_.end());
    pending_.clear();
    for (void* ptr : snapshot) {
        qjs::JSEngine::PromiseHandle h{ptr};
        engine.rejectPromise(h, message, code);
        engine.freePromise(h);
    }
}

} // namespace qianjs
