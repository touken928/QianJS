#include "runtime/instance.h"

#include "runtime/embed.h"
#include "runtime/plugin_lifecycle.h"

#if QIANJS_MODULE_UI
#include "platform/platform_window.h"

namespace qianjs {
bool run_frame_loop_impl(RuntimeInstance& instance, AppHost& app, const FrameLoopOptions& options);
}
#endif

#include <iostream>
#include <thread>

namespace qianjs {

thread_local RuntimeInstance* RuntimeInstance::current_ = nullptr;

RuntimeInstance::RuntimeInstance() {
    scheduler_.bind_runtime(this);
}

RuntimeInstance::~RuntimeInstance() {
    deactivate();
    if (phase_ != LifecyclePhase::Destroyed && phase_ != LifecyclePhase::Created) {
        shutdown();
    }
}

void RuntimeInstance::activate() {
    current_ = this;
}

void RuntimeInstance::begin_script_execution() {
    activate();
    phase_ = LifecyclePhase::Running;
    scheduler_.set_phase(LifecyclePhase::Running);
}

void RuntimeInstance::deactivate() {
    if (current_ == this) {
        current_ = nullptr;
    }
}

RuntimeInstance* RuntimeInstance::current() {
    return current_;
}

void RuntimeInstance::initialize(const qjs::PluginRegistry& plugins) {
    if (phase_ != LifecyclePhase::Created) {
        return;
    }

    engine_.setHost<RuntimeInstance>(this);
    plugins.installAll(engine_.context(), engine_.modules());

    scheduler_.set_phase(LifecyclePhase::Initialized);
    phase_ = LifecyclePhase::Initialized;
    scheduler_.ensure_started();
}

void RuntimeInstance::notify_plugins_shutdown() {
    notify_lifecycle(LifecyclePhase::Shutdown, *this);
}

#if QIANJS_MODULE_UI

platform::PlatformWindow& RuntimeInstance::ensure_window() {
    if (!window_) {
        window_ = std::make_unique<platform::PlatformWindow>();
    }
    return *window_;
}

void RuntimeInstance::destroy_window() {
    if (window_) {
        window_->destroy();
        window_.reset();
    }
}

bool RuntimeInstance::run_frame_loop(AppHost& app, const FrameLoopOptions& options) {
    return qianjs::run_frame_loop_impl(*this, app, options);
}

void RuntimeInstance::set_deferred_app(qjs::Value app_obj) {
    clear_deferred_app();
    if (!deferred_app_.load_from_object(std::move(app_obj))) {
        return;
    }
    has_deferred_app_ = deferred_app_.has_hooks();
}

void RuntimeInstance::clear_deferred_app() {
    if (!has_deferred_app_) {
        return;
    }
    deferred_app_.release();
    has_deferred_app_ = false;
}

bool RuntimeInstance::try_run_deferred_app(const FrameLoopOptions& options) {
    if (!has_deferred_app_) {
        return false;
    }
    const bool ok = run_frame_loop(deferred_app_, options);
    has_deferred_app_ = false;
    return ok;
}

#endif

std::unique_ptr<qjs::Promise> RuntimeInstance::create_promise() {
    auto p = engine_.createPromise();
    if (p) {
        promises_.track(p.get());
    }
    return p;
}

void RuntimeInstance::track_promise(qjs::Promise* p) {
    promises_.track(p);
}

void RuntimeInstance::untrack_promise(qjs::Promise* p) {
    promises_.untrack(p);
}

void RuntimeInstance::reject_pending_promises() {
    promises_.reject_all("Runtime shut down", "ESHUTDOWN");
}

void RuntimeInstance::shutdown() {
    if (phase_ == LifecyclePhase::Destroyed || phase_ == LifecyclePhase::Created) {
        return;
    }

    deactivate();
    ++generation_;

#if QIANJS_MODULE_UI
    clear_deferred_app();
#endif
    reject_pending_promises();

    notify_plugins_shutdown();

    scheduler_.set_phase(LifecyclePhase::Shutdown);
    phase_ = LifecyclePhase::Shutdown;
    scheduler_.shutdown(engine_);

    phase_ = LifecyclePhase::Destroyed;
}

bool RuntimeInstance::run_file(const std::filesystem::path& path) {
    if (phase_ != LifecyclePhase::Initialized) {
        return false;
    }
    const std::string code = Embed::readTextFile(path);
    if (code.empty()) {
        return false;
    }
    begin_script_execution();
    const bool ok = engine_.evalModule(path.string(), code).success;
    if (!ok) {
        enter_draining();
        run_until_idle(std::chrono::seconds(5));
    }
    return ok;
}

bool RuntimeInstance::run_bytecode(const uint8_t* data, size_t len) {
    if (phase_ != LifecyclePhase::Initialized) {
        return false;
    }
    begin_script_execution();
    const bool ok = engine_.runBytecode(data, len).success;
    if (!ok) {
        enter_draining();
        run_until_idle(std::chrono::seconds(5));
    }
    return ok;
}

void RuntimeInstance::pump_async() {
    if (!phase_allows_js(phase_)) {
        return;
    }
    scheduler_.tick();
    scheduler_.run_deferred(engine_);
}

void RuntimeInstance::pump_microtasks() {
    if (!phase_allows_js(phase_)) {
        return;
    }
    engine_.pumpMicrotasks();
}

void RuntimeInstance::pump_once() {
    pump_async();
    pump_microtasks();
}

bool RuntimeInstance::is_idle() const {
    return !scheduler_.has_pending_work() && !engine_.isJobPending();
}

void RuntimeInstance::enter_draining() {
    if (phase_ == LifecyclePhase::Running) {
        phase_ = LifecyclePhase::Draining;
        scheduler_.set_phase(LifecyclePhase::Draining);
    }
}

void RuntimeInstance::run_until_idle(std::chrono::milliseconds timeout) {
    enter_draining();
    activate();

    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + timeout;
    int idle_rounds = 0;

    while (clock::now() < deadline) {
        pump_once();

        if (is_idle()) {
            if (++idle_rounds >= 3) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        idle_rounds = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cerr << "Warning: timed out while waiting for pending native I/O to finish\n";
}

} // namespace qianjs
