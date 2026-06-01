#pragma once

#include "runtime/app_host.h"
#include "runtime/lifecycle.h"
#include "runtime/promise_registry.h"
#include "runtime/runtime_context.h"
#include "runtime/scheduler.h"

#include <qianjs_modules.h>

#include <qjs/engine.h>
#include <qjs/plugin.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace qianjs {

namespace platform {
#if QIANJS_MODULE_CANVAS
class CanvasRegistry;
#endif
#if QIANJS_MODULE_CANVAS || QIANJS_MODULE_GAME
class PlatformCanvas;
#endif
}

/**
 * Owns one script run: Engine, host context, scheduler, and lifecycle phase.
 * All native async work (timers, fs defer) must run while this instance is active().
 */
class RuntimeInstance {
public:
    RuntimeInstance();
    ~RuntimeInstance();

    RuntimeInstance(const RuntimeInstance&) = delete;
    RuntimeInstance& operator=(const RuntimeInstance&) = delete;

    void initialize();
    void shutdown();

    qjs::Engine& engine() { return engine_; }
    const qjs::Engine& engine() const { return engine_; }
    RuntimeContext& host() { return host_; }
    const RuntimeContext& host() const { return host_; }
    Scheduler& scheduler() { return scheduler_; }
    const Scheduler& scheduler() const { return scheduler_; }

    LifecyclePhase phase() const { return phase_; }
    bool is_running() const { return phase_is_running(phase_); }
    uint64_t generation() const { return generation_; }

    void activate();
    void deactivate();
    void begin_script_execution();

    static RuntimeInstance* current();

    bool run_file(const std::filesystem::path& path);
    bool run_bytecode(const uint8_t* data, size_t len);

    /** libuv tick + flush defer queue (macrotasks). */
    void pump_async();
    /** QuickJS microtask queue only. */
    void pump_microtasks();
    /** `pump_async` + `pump_microtasks` (CLI drain path). */
    void pump_once();

    void run_until_idle(std::chrono::milliseconds timeout = std::chrono::seconds(120));

    void enter_draining();

    std::unique_ptr<qjs::Promise> create_promise();
    void track_promise(qjs::Promise* p);
    void untrack_promise(qjs::Promise* p);

#if QIANJS_MODULE_CANVAS
    platform::CanvasRegistry& canvases() { return *canvases_; }
    const platform::CanvasRegistry& canvases() const { return *canvases_; }
#endif

#if QIANJS_MODULE_GAME
    bool run_frame_loop(platform::PlatformCanvas& canvas, AppHost& app, const FrameLoopOptions& options);

    void set_deferred_app(uint64_t canvas_id, qjs::Value app_obj);
    void clear_deferred_app();
    bool has_deferred_app() const { return has_deferred_app_; }
    bool try_run_deferred_app(const FrameLoopOptions& options = {});
#endif

    void notify_plugins_shutdown();

private:
    bool is_idle() const;
    void reject_pending_promises();

    qjs::Engine engine_;
    RuntimeContext host_;
    Scheduler scheduler_;
    PromiseRegistry promises_;
    LifecyclePhase phase_ = LifecyclePhase::Created;
    uint64_t generation_ = 0;

#if QIANJS_MODULE_CANVAS
    std::unique_ptr<platform::CanvasRegistry> canvases_;
#endif

#if QIANJS_MODULE_GAME
    uint64_t deferred_canvas_id_ = 0;
    AppHost deferred_app_;
    bool has_deferred_app_ = false;
#endif

    static thread_local RuntimeInstance* current_;
};

} // namespace qianjs
