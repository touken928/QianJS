#pragma once

#include <qjs/promise.h>

#include <unordered_set>
#include <vector>

namespace qianjs {

/** Tracks native promises until resolve/reject + release. */
class PromiseRegistry {
public:
    void track(qjs::Promise* p);
    void untrack(qjs::Promise* p);
    void reject_all(const char* message, const char* code = "ESHUTDOWN");

private:
    std::unordered_set<qjs::Promise*> pending_;
};

} // namespace qianjs
