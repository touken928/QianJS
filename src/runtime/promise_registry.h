#pragma once

#include <js_engine.h>

#include <cstddef>
#include <unordered_set>
#include <vector>

namespace qianjs {

/** Tracks native PromiseHandle ptrs until resolve/reject + release. */
class PromiseRegistry {
public:
    void track(qjs::JSEngine::PromiseHandle h);
    void untrack(qjs::JSEngine::PromiseHandle h);
    void reject_all(qjs::JSEngine& engine, const char* message, const char* code = "ESHUTDOWN");

private:
    std::unordered_set<void*> pending_;
};

} // namespace qianjs
