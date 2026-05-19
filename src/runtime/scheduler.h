#pragma once

#include "runtime/lifecycle.h"

#include <js_engine.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#if QIANJS_HAVE_LIBUV

namespace uvw {
class loop;
}

struct uv_loop_s;
typedef struct uv_loop_s uv_loop_t;
struct uv_timer_s;
typedef struct uv_timer_s uv_timer_t;

#endif

namespace qianjs {

class RuntimeInstance;

class Scheduler {
public:
    Scheduler();
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void bind_runtime(RuntimeInstance* owner) { owner_ = owner; }

    void set_phase(LifecyclePhase phase) { phase_ = phase; }
    LifecyclePhase phase() const { return phase_; }

#if QIANJS_HAVE_LIBUV
    void ensure_started();
    void tick();
    std::shared_ptr<uvw::loop> uvw_loop();
    uv_loop_t* raw_loop();
#else
    void ensure_started() {}
    void tick() {}
#endif

    void defer(std::function<void(qjs::JSEngine&)> fn);
    void run_deferred(qjs::JSEngine& engine);

    void begin_operation();
    void end_operation();
    int pending_operations() const;
    bool has_pending_deferred() const;
    /** Native I/O/timer ops plus queued defer callbacks. */
    bool has_pending_work() const;

    /** Returns false if new timers are not accepted (Draining/Shutdown). */
    bool add_timer(JSContext* c, JSValue callback, int64_t delay_ms, bool repeat, int64_t& out_id);
    void cancel_timer(int64_t id);

    /** Stop timers, drop deferred queue; free timer callbacks on `engine` when ctx is valid. */
    void shutdown(qjs::JSEngine& engine);

private:
    struct TimerEntry;

    void run_timer_callback(qjs::JSEngine& engine, const std::shared_ptr<TimerEntry>& entry);
    void free_timer_callback(qjs::JSEngine& engine, const std::shared_ptr<TimerEntry>& entry);

#if QIANJS_HAVE_LIBUV
    static void on_uv_timer(uv_timer_t* handle);
    void stop_uv_timer(const std::shared_ptr<TimerEntry>& entry);
#endif

    LifecyclePhase phase_ = LifecyclePhase::Created;
    RuntimeInstance* owner_ = nullptr;

    mutable std::mutex defer_mu_;
    std::vector<std::function<void(qjs::JSEngine&)>> defer_pending_;

    std::atomic<int> pending_ops_{0};

    std::mutex timer_mu_;
    std::unordered_map<int64_t, std::shared_ptr<TimerEntry>> timers_;
    int64_t next_timer_id_ = 1;

#if QIANJS_HAVE_LIBUV
    std::mutex loop_mu_;
    std::shared_ptr<uvw::loop> uv_loop_;
#endif
};

} // namespace qianjs
