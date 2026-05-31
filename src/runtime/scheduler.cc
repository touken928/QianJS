#include "runtime/scheduler.h"

#include "runtime/instance.h"

#include <atomic>
#include <iostream>
#include <utility>

#if QIANJS_HAVE_LIBUV
#include <uv.h>
#include <uvw.hpp>
#endif

namespace qianjs {

struct Scheduler::TimerEntry {
    Scheduler* owner = nullptr;
    int64_t id = 0;
    int64_t delay_ms = 0;
    bool repeat = false;
    bool cancelled = false;
    qjs::Value callback{};
#if QIANJS_HAVE_LIBUV
    uv_timer_t uv{};
    bool uv_started = false;
#endif
};

Scheduler::Scheduler() = default;

Scheduler::~Scheduler() {
#if QIANJS_HAVE_LIBUV
    std::lock_guard<std::mutex> lock(timer_mu_);
    for (auto& [_, entry] : timers_) {
        if (entry && entry->uv_started) {
            uv_timer_stop(&entry->uv);
            entry->uv_started = false;
        }
    }
    timers_.clear();
#endif
}

#if QIANJS_HAVE_LIBUV

void Scheduler::ensure_started() {
    std::lock_guard<std::mutex> lock(loop_mu_);
    if (!uv_loop_)
        uv_loop_ = uvw::loop::create();
}

std::shared_ptr<uvw::loop> Scheduler::uvw_loop() {
    ensure_started();
    std::lock_guard<std::mutex> lock(loop_mu_);
    return uv_loop_;
}

uv_loop_t* Scheduler::raw_loop() {
    return uvw_loop()->raw();
}

void Scheduler::tick() {
    ensure_started();
    std::lock_guard<std::mutex> lock(loop_mu_);
    if (uv_loop_)
        uv_loop_->run(uvw::loop::run_mode::NOWAIT);
}

#endif // QIANJS_HAVE_LIBUV

void Scheduler::defer(std::function<void(qjs::Engine&)> fn) {
    if (!phase_allows_js(phase_)) {
        return;
    }
    const uint64_t gen = owner_ ? owner_->generation() : 0;
    RuntimeInstance* owner = owner_;
    std::lock_guard<std::mutex> lock(defer_mu_);
    defer_pending_.push_back(
        [gen, owner, fn = std::move(fn)](qjs::Engine& e) mutable {
            if (owner && owner->generation() != gen) {
                return;
            }
            fn(e);
        });
}

void Scheduler::run_deferred(qjs::Engine& engine) {
    if (!phase_allows_js(phase_)) {
        return;
    }
    std::vector<std::function<void(qjs::Engine&)>> batch;
    {
        std::lock_guard<std::mutex> lock(defer_mu_);
        batch.swap(defer_pending_);
    }
    for (auto& f : batch) {
        if (!phase_allows_js(phase_)) {
            break;
        }
        f(engine);
    }
}

void Scheduler::begin_operation() {
    pending_ops_.fetch_add(1, std::memory_order_relaxed);
}

void Scheduler::end_operation() {
    pending_ops_.fetch_sub(1, std::memory_order_relaxed);
}

int Scheduler::pending_operations() const {
    return pending_ops_.load(std::memory_order_relaxed);
}

bool Scheduler::has_pending_work() const {
    return pending_operations() > 0 || has_pending_deferred();
}

bool Scheduler::has_pending_deferred() const {
    std::lock_guard<std::mutex> lock(defer_mu_);
    return !defer_pending_.empty();
}

#if QIANJS_HAVE_LIBUV

void Scheduler::stop_uv_timer(const std::shared_ptr<TimerEntry>& entry) {
    if (!entry || !entry->uv_started) {
        return;
    }
    uv_timer_stop(&entry->uv);
    entry->uv_started = false;
}

void Scheduler::on_uv_timer(uv_timer_t* handle) {
    if (!handle || !handle->data) {
        return;
    }
    auto* entry = static_cast<TimerEntry*>(handle->data);
    Scheduler* self = entry->owner;
    if (!self || entry->cancelled) {
        return;
    }
    std::shared_ptr<TimerEntry> shared_entry;
    {
        std::lock_guard<std::mutex> lock(self->timer_mu_);
        auto it = self->timers_.find(entry->id);
        if (it != self->timers_.end()) {
            shared_entry = it->second;
        }
    }
    if (!shared_entry) {
        return;
    }
    self->defer([self, shared_entry](qjs::Engine& e) { self->run_timer_callback(e, shared_entry); });
}

#endif

void Scheduler::run_timer_callback(qjs::Engine& engine, const std::shared_ptr<TimerEntry>& entry) {
    if (!entry || entry->cancelled || !phase_allows_js(phase_)) {
        return;
    }
    if (!entry->callback.isFunction()) {
        return;
    }

    auto ret = engine.call(entry->callback);
    if (!ret.success) {
        std::cerr << "timers callback exception\n";
    }

    if (!entry->repeat) {
        entry->cancelled = true;
#if QIANJS_HAVE_LIBUV
        stop_uv_timer(entry);
#endif
        {
            std::lock_guard<std::mutex> lock(timer_mu_);
            auto it = timers_.find(entry->id);
            if (it != timers_.end() && it->second.get() == entry.get()) {
                timers_.erase(it);
            }
        }
        entry->callback = {};
        end_operation();
    }
}

bool Scheduler::add_timer(qjs::Value callback, int64_t delay_ms, bool repeat, int64_t& out_id) {
    if (!phase_allows_new_async(phase_)) {
        return false;
    }

#if !QIANJS_HAVE_LIBUV
    (void)callback;
    (void)delay_ms;
    (void)repeat;
    (void)out_id;
    return false;
#else
    ensure_started();

    if (delay_ms < 0) {
        delay_ms = 0;
    }
    if (repeat && delay_ms == 0) {
        delay_ms = 1;
    }

    auto entry = std::make_shared<TimerEntry>();
    entry->owner = this;
    entry->id = next_timer_id_++;
    entry->delay_ms = delay_ms;
    entry->repeat = repeat;
    entry->callback = std::move(callback);

    uv_loop_t* loop = raw_loop();
    if (uv_timer_init(loop, &entry->uv) != 0) {
        entry->callback = {};
        return false;
    }
    entry->uv.data = entry.get();

    const uint64_t delay = static_cast<uint64_t>(delay_ms);
    const uint64_t repeat_ms = repeat ? static_cast<uint64_t>(delay_ms) : 0;
    if (uv_timer_start(&entry->uv, &Scheduler::on_uv_timer, delay, repeat_ms) != 0) {
        entry->callback = {};
        return false;
    }
    entry->uv_started = true;

    {
        std::lock_guard<std::mutex> lock(timer_mu_);
        timers_[entry->id] = entry;
    }

    begin_operation();
    out_id = entry->id;
    return true;
#endif
}

void Scheduler::cancel_timer(int64_t id) {
    std::shared_ptr<TimerEntry> entry;
    {
        std::lock_guard<std::mutex> lock(timer_mu_);
        auto it = timers_.find(id);
        if (it == timers_.end()) {
            return;
        }
        entry = it->second;
        timers_.erase(it);
    }

    if (!entry || entry->cancelled) {
        return;
    }
    entry->cancelled = true;
#if QIANJS_HAVE_LIBUV
    stop_uv_timer(entry);
#endif
    end_operation();
    defer([entry](qjs::Engine&) { entry->callback = {}; });
}

void Scheduler::shutdown(qjs::Engine& engine) {
    phase_ = LifecyclePhase::Shutdown;

    std::vector<std::shared_ptr<TimerEntry>> remaining;
    {
        std::lock_guard<std::mutex> lock(timer_mu_);
        remaining.reserve(timers_.size());
        for (auto& [_, t] : timers_) {
            remaining.push_back(t);
        }
        timers_.clear();
    }

    for (auto& entry : remaining) {
        if (!entry) {
            continue;
        }
        entry->cancelled = true;
#if QIANJS_HAVE_LIBUV
        stop_uv_timer(entry);
#endif
        entry->callback = {};
    }

    {
        std::lock_guard<std::mutex> lock(defer_mu_);
        defer_pending_.clear();
    }

    pending_ops_.store(0, std::memory_order_relaxed);
}

} // namespace qianjs
