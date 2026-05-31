#include <qianjs_modules.h>

#if !QIANJS_MODULE_GAME
#error "frame_loop.cc requires QIANJS_MODULE_GAME"
#endif

#include "platform/platform_canvas.h"
#include "runtime/app_host.h"
#include "runtime/clock.h"
#include "runtime/instance.h"
#include "systems/input_system.h"
#include "systems/render_system.h"

#include <qjs/engine.h>

#include <iostream>
#include <thread>
#include <vector>

namespace qianjs {

namespace {

bool call_void(qjs::Engine& engine, const qjs::Value& fn) {
    if (!fn.isFunction()) {
        return true;
    }
    return engine.call(fn).success;
}

bool call_update(qjs::Engine& engine, const qjs::Value& fn, double dt, const qjs::Value& input) {
    return engine.call(fn, engine.float64(dt), input).success;
}

} // namespace

bool run_frame_loop_impl(RuntimeInstance& instance, platform::PlatformCanvas& canvas, AppHost& app,
    const FrameLoopOptions& options) {
    if (!app.has_hooks()) {
        return false;
    }

    qjs::Engine& engine = instance.engine();
    if (!instance.is_running()) {
        instance.begin_script_execution();
    }

    if (app.init_fn().isFunction() && !call_void(engine, app.init_fn())) {
        std::cerr << "app init error\n";
        app.release();
        return false;
    }
    instance.pump_microtasks();

    Clock clock;
    clock.reset();
    if (options.fixed_dt > 0) {
        clock.set_fixed_dt(options.fixed_dt);
    }

    systems::InputSystem input_sys;
    systems::RenderSystem render_sys;

    bool running = true;
    int64_t frame = 0;

    while (running) {
        const double real_dt = clock.tick();

        if (options.target_fps > 0) {
            const double frame_budget = 1.0 / options.target_fps;
            if (real_dt < frame_budget) {
                const auto sleep_ms = static_cast<int>((frame_budget - real_dt) * 1000.0);
                if (sleep_ms > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                }
            }
        }

        std::vector<SDL_Event> batch;
        bool quit = false;
        input_sys.poll(canvas, batch, quit);
        if (quit) {
            running = false;
        }

        instance.pump_async();

        systems::InputFrame input_frame{frame, real_dt, clock.alpha()};
        qjs::Value input = input_sys.build_input_object(engine, canvas, batch, input_frame);
        if (!input.valid()) {
            std::cerr << "app input build error\n";
            running = false;
            break;
        }

        const int fixed_steps = clock.consume_fixed_steps();
        if (fixed_steps > 0) {
            for (int s = 0; s < fixed_steps; ++s) {
                if (!call_update(engine, app.update_fn(), clock.fixed_dt(), input)) {
                    std::cerr << "app update error\n";
                    running = false;
                    break;
                }
                instance.pump_microtasks();
            }
        } else if (!call_update(engine, app.update_fn(), real_dt, input)) {
            std::cerr << "app update error\n";
            running = false;
        }

        if (running) {
            instance.pump_microtasks();
            if (!render_sys.render_frame(canvas, engine, app.render_fn())) {
                std::cerr << "app render error\n";
                running = false;
            }
        }

        ++frame;
        if (options.max_frames > 0 && frame >= options.max_frames) {
            running = false;
        }
    }

    if (app.shutdown_fn().isFunction()) {
        if (!call_void(engine, app.shutdown_fn())) {
            std::cerr << "app shutdown error\n";
        }
        instance.pump_once();
    }

    app.release();
    instance.enter_draining();
    return true;
}

} // namespace qianjs
