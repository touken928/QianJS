#include "runtime/promise_registry.h"

namespace qianjs {

void PromiseRegistry::track(qjs::Promise* p) {
    if (p) {
        pending_.insert(p);
    }
}

void PromiseRegistry::untrack(qjs::Promise* p) {
    if (p) {
        pending_.erase(p);
    }
}

void PromiseRegistry::reject_all(const char* message, const char* code) {
    std::vector<qjs::Promise*> copy(pending_.begin(), pending_.end());
    for (qjs::Promise* p : copy) {
        if (p) {
            p->reject(message, code ? code : "");
        }
    }
    pending_.clear();
}

} // namespace qianjs
