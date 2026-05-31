#pragma once

#include <qjs/engine.h>

#include <functional>
#include <memory>

#if QIANJS_HAVE_LIBUV

namespace uvw {
class loop;
}

struct uv_loop_s;
typedef struct uv_loop_s uv_loop_t;

namespace qianjs::event_loop::uv {

/** Shared uvw loop owned by the runtime (`event_loop.cc`). */
std::shared_ptr<uvw::loop> uvw_loop();

/** Underlying `uv_loop_t*`; same lifetime as `uvw_loop()`. */
uv_loop_t* loop();

} // namespace qianjs::event_loop::uv

#endif

namespace qianjs::event_loop {

/**
 * Host-side event driver. With `QIANJS_HAVE_LIBUV`, backed by uvw/libuv; otherwise `tick` / `ensure_started` are no-ops.
 * Native I/O code includes this header for `defer` / `tick` / `uvw_loop()`, and `<uvw.hpp>` for uvw types when libuv is enabled.
 */

/** Idempotent; ensures the loop exists when libuv is enabled. */
void ensure_started();

/** One non-blocking pass when libuv is enabled (`UV_RUN_NOWAIT`); otherwise a no-op. */
void tick();

/**
 * Queue work that must run on the JS thread (Promise settle, engine APIs).
 * Safe to call from libuv callbacks when enabled; runs on the next `run_deferred`.
 */
void defer(std::function<void(qjs::Engine&)> fn);

void run_deferred(qjs::Engine& engine);

/** Book-keeping for fs async ops so the host knows when the process can go idle. */
void begin_operation();
void end_operation();
int pending_operations();

void shutdown();

} // namespace qianjs::event_loop
