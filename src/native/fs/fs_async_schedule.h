#pragma once

#include "runtime/event_loop/event_loop.h"
#include "runtime/promise_track.h"

#include <qjs/engine.h>

#include <memory>
#include <string>

namespace qianjs::fs::schedule {

inline void reject(std::shared_ptr<qjs::Promise> ph, std::string msg, const std::string& code = {}) {
    qianjs::event_loop::defer(
        [ph, msg = std::move(msg), code](qjs::Engine& e) {
            if (ph) {
                ph->reject(msg, code);
                qianjs::promise::release(ph.get());
            }
            (void)e;
        });
}

inline void resolve_void(std::shared_ptr<qjs::Promise> ph) {
    qianjs::event_loop::defer([ph](qjs::Engine& e) {
        if (ph) {
            ph->resolveVoid();
            qianjs::promise::release(ph.get());
        }
        (void)e;
    });
}

inline void resolve_string(std::shared_ptr<qjs::Promise> ph, std::string data) {
    qianjs::event_loop::defer(
        [ph, data = std::move(data)](qjs::Engine& e) {
            if (ph) {
                ph->resolveString(data);
                qianjs::promise::release(ph.get());
            }
            (void)e;
        });
}

inline void resolve_bytes(std::shared_ptr<qjs::Promise> ph, std::string buffer) {
    qianjs::event_loop::defer(
        [ph, buf = std::move(buffer)](qjs::Engine& e) {
            if (ph) {
                ph->resolveBytes(reinterpret_cast<const uint8_t*>(buf.data()), buf.size());
                qianjs::promise::release(ph.get());
            }
            (void)e;
        });
}

} // namespace qianjs::fs::schedule
