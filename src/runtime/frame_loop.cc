#include <qianjs_modules.h>

#if !QIANJS_MODULE_UI
#error "frame_loop.cc requires QIANJS_MODULE_UI"
#endif

#include "platform/platform_window.h"
#include "runtime/app_host.h"
#include "runtime/clock.h"
#include "runtime/instance.h"
#include "systems/input_system.h"
#include "systems/render_system.h"

#include <js_engine.h>

#include <iostream>
#include <thread>
#include <vector>

namespace qianjs {

namespace {

bool call_void(JSContext* c, JSValue fn) {
    if (JS_IsUndefined(fn)) {
        return true;
    }
    JSValue r = JS_Call(c, fn, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(r)) {
        return false;
    }
    JS_FreeValue(c, r);
    return true;
}

bool call_update(JSContext* c, JSValue fn, double dt, JSValue input) {
    JSValue args[2] = {JS_NewFloat64(c, dt), input};
    if (JS_IsException(args[0])) {
        return false;
    }
    JSValue r = JS_Call(c, fn, JS_UNDEFINED, 2, args);
    JS_FreeValue(c, args[0]);
    if (JS_IsException(r)) {
        return false;
    }
    JS_FreeValue(c, r);
    return true;
}

void dump_exception(qjs::JSEngine& engine) {
    JSContext* c = engine.ctx();
    JSValue exc = JS_GetException(c);
    const char* msg = JS_ToCString(c, exc);
    if (msg) {
        std::cerr << "app error: " << msg << '\n';
        JS_FreeCString(c, msg);
    }
    JS_FreeValue(c, exc);
}

} // namespace

bool run_frame_loop_impl(RuntimeInstance& instance, AppHost& app, const FrameLoopOptions& options) {
    if (!app.has_hooks()) {
        return false;
    }

    platform::PlatformWindow& win = instance.ensure_window();
    if (!win.init(options.width, options.height, options.title ? options.title : "QianJS")) {
        std::cerr << "runApp: failed to create window\n";
        return false;
    }

    qjs::JSEngine& engine = instance.engine();
    JSContext* c = engine.ctx();
    if (!instance.is_running()) {
        instance.begin_script_execution();
    }

    if (!JS_IsUndefined(app.init_fn()) && !call_void(c, app.init_fn())) {
        dump_exception(engine);
        app.release(c);
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
        input_sys.poll(win, batch, quit);
        if (quit) {
            running = false;
        }

        instance.pump_async();

        systems::InputFrame input_frame{frame, real_dt, clock.alpha()};
        JSValue input = input_sys.build_input_object(c, batch, input_frame);
        if (JS_IsException(input)) {
            dump_exception(engine);
            running = false;
            break;
        }

        const int fixed_steps = clock.consume_fixed_steps();
        if (fixed_steps > 0) {
            for (int s = 0; s < fixed_steps; ++s) {
                if (!call_update(c, app.update_fn(), clock.fixed_dt(), input)) {
                    dump_exception(engine);
                    running = false;
                    break;
                }
                instance.pump_microtasks();
            }
        } else if (!call_update(c, app.update_fn(), real_dt, input)) {
            dump_exception(engine);
            running = false;
        }

        if (running) {
            instance.pump_microtasks();
            if (!render_sys.render_frame(win, c, app.render_fn())) {
                dump_exception(engine);
                running = false;
            }
        }

        JS_FreeValue(c, input);

        ++frame;
        if (options.max_frames > 0 && frame >= options.max_frames) {
            running = false;
        }
    }

    if (!JS_IsUndefined(app.shutdown_fn())) {
        if (!call_void(c, app.shutdown_fn())) {
            dump_exception(engine);
        }
        instance.pump_once();
    }

    app.release(c);
    instance.enter_draining();
    return true;
}

} // namespace qianjs
