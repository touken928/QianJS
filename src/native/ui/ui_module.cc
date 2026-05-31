#include "native/ui/ui_module.h"

#include "platform/ui_access.h"
#include "runtime/app_host.h"
#include "runtime/instance.h"

#include <qjs/call.h>
#include <qjs/engine.h>
#include <qjs/module.h>
#include <qjs/object.h>

#include <cmath>
#include <string>
#include <vector>

namespace {

using qianjs::platform::require_window;

bool parse_frame_loop_options(qjs::CallContext& ctx, qianjs::FrameLoopOptions& opts, std::string& title_storage) {
    if (ctx.argc() < 2) {
        return true;
    }
    auto optVal = ctx.valueArg(1);
    if (!optVal.success) {
        return false;
    }
    if (optVal.value.isUndefined()) {
        return true;
    }
    if (!optVal.value.isObject()) {
        return false;
    }

    auto read_opt_int = [&](const char* key, int* out) -> bool {
        auto v = optVal.value.getProperty(key);
        if (!v.success) {
            return false;
        }
        if (v.value.isUndefined()) {
            return true;
        }
        auto n = v.value.toInt32();
        if (!n.success) {
            return false;
        }
        *out = n.value;
        return true;
    };

    auto read_opt_i64 = [&](const char* key, int64_t* out) -> bool {
        auto v = optVal.value.getProperty(key);
        if (!v.success) {
            return false;
        }
        if (v.value.isUndefined()) {
            return true;
        }
        auto n = v.value.toInt64();
        if (!n.success) {
            return false;
        }
        *out = n.value;
        return true;
    };

    auto read_opt_double = [&](const char* key, double* out) -> bool {
        auto v = optVal.value.getProperty(key);
        if (!v.success) {
            return false;
        }
        if (v.value.isUndefined()) {
            return true;
        }
        auto n = v.value.toFloat64();
        if (!n.success) {
            return false;
        }
        *out = n.value;
        return true;
    };

    auto read_opt_string = [&](const char* key, std::string* out) -> bool {
        auto v = optVal.value.getProperty(key);
        if (!v.success) {
            return false;
        }
        if (v.value.isUndefined()) {
            return true;
        }
        auto s = v.value.toString();
        if (!s.success) {
            return false;
        }
        *out = std::move(s.value);
        return true;
    };

    if (!read_opt_int("width", &opts.width)) {
        return false;
    }
    if (!read_opt_int("height", &opts.height)) {
        return false;
    }
    if (!read_opt_i64("maxFrames", &opts.max_frames)) {
        return false;
    }
    if (!read_opt_double("fps", &opts.target_fps)) {
        return false;
    }
    if (!read_opt_double("fixedStep", &opts.fixed_dt)) {
        return false;
    }
    if (!read_opt_string("title", &title_storage)) {
        return false;
    }
    if (!title_storage.empty()) {
        opts.title = title_storage.c_str();
    }
    return true;
}

} // namespace

const char* UiPlugin::name() const {
    return "ui";
}

void UiPlugin::install(qjs::Context&, qjs::Module& root) {
    auto& m = root.module("ui");

    m.funcDynamic("init", 2, 3, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto w = ctx.int32Arg(0);
        if (!w.success) {
            return qjs::Result<qjs::Value>::fail(w.error);
        }
        auto h = ctx.int32Arg(1);
        if (!h.success) {
            return qjs::Result<qjs::Value>::fail(h.error);
        }
        if (w.value <= 0 || h.value <= 0) {
            return qjs::Result<qjs::Value>::fail(
                qjs::ErrorInfo{"ui.init: width and height must be positive", {}, {}});
        }

        std::string title = "QianJS";
        if (ctx.argc() >= 3) {
            auto t = ctx.stringArg(2);
            if (!t.success) {
                return qjs::Result<qjs::Value>::fail(t.error);
            }
            title = std::move(t.value);
        }

        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui.init: no active runtime instance", {}, {}});
        }
        if (!inst->ensure_window().init(w.value, h.value, title)) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui.init: SDL initialization failed", {}, {}});
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("close", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
            inst->destroy_window();
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("pollEvents", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        std::vector<SDL_Event> batch;
        if (auto* win = require_window(ctx)) {
            win->poll_events(batch);
        } else {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        return qjs::Result<qjs::Value>::ok(
            ctx.engine().boolValue(qianjs::platform::PlatformWindow::batch_quit_or_escape(batch)));
    });

    m.funcDynamic("readEvents", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        std::vector<SDL_Event> batch;
        win->poll_events(batch);
        const bool quit = qianjs::platform::PlatformWindow::batch_quit_or_escape(batch);
        return qjs::Result<qjs::Value>::ok(ctx.engine().object()
                                               .setBool("quit", quit)
                                               .set("events", qianjs::platform::PlatformWindow::events_to_js(ctx.engine(), batch))
                                               .build());
    });

    m.funcDynamic("getMouseState", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        return qjs::Result<qjs::Value>::ok(win->mouse_state_js(ctx.engine()));
    });

    m.funcDynamic("getModState", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        return qjs::Result<qjs::Value>::ok(win->mod_state_js(ctx.engine()));
    });

    m.funcDynamic("isKeyDown", 1, 1, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        auto key = ctx.valueArg(0);
        if (!key.success) {
            return qjs::Result<qjs::Value>::fail(key.error);
        }
        return qjs::Result<qjs::Value>::ok(win->is_key_down_js(ctx.engine(), key.value));
    });

    m.funcDynamic("present", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        win->present();
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("setSourceRGBA", 4, 4, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        auto r = ctx.float64Arg(0);
        auto g = ctx.float64Arg(1);
        auto b = ctx.float64Arg(2);
        auto a = ctx.float64Arg(3);
        if (!r.success || !g.success || !b.success || !a.success) {
            return qjs::Result<qjs::Value>::fail(
                !r.success ? r.error : !g.success ? g.error : !b.success ? b.error : a.error);
        }
        win->set_color(static_cast<float>(r.value), static_cast<float>(g.value), static_cast<float>(b.value),
            static_cast<float>(a.value));
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("clear", 4, 4, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        auto r = ctx.float64Arg(0);
        auto g = ctx.float64Arg(1);
        auto b = ctx.float64Arg(2);
        auto a = ctx.float64Arg(3);
        if (!r.success || !g.success || !b.success || !a.success) {
            return qjs::Result<qjs::Value>::fail(
                !r.success ? r.error : !g.success ? g.error : !b.success ? b.error : a.error);
        }
        win->clear_framebuffer(static_cast<float>(r.value), static_cast<float>(g.value), static_cast<float>(b.value),
            static_cast<float>(a.value));
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("fillRect", 4, 4, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        auto x = ctx.float64Arg(0);
        auto y = ctx.float64Arg(1);
        auto rw = ctx.float64Arg(2);
        auto rh = ctx.float64Arg(3);
        if (!x.success || !y.success || !rw.success || !rh.success) {
            return qjs::Result<qjs::Value>::fail(
                !x.success ? x.error : !y.success ? y.error : !rw.success ? rw.error : rh.error);
        }
        win->fill_rect(static_cast<float>(x.value), static_cast<float>(y.value), static_cast<float>(rw.value),
            static_cast<float>(rh.value));
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("strokeRect", 4, 4, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        auto x = ctx.float64Arg(0);
        auto y = ctx.float64Arg(1);
        auto rw = ctx.float64Arg(2);
        auto rh = ctx.float64Arg(3);
        if (!x.success || !y.success || !rw.success || !rh.success) {
            return qjs::Result<qjs::Value>::fail(
                !x.success ? x.error : !y.success ? y.error : !rw.success ? rw.error : rh.error);
        }
        win->stroke_rect(static_cast<float>(x.value), static_cast<float>(y.value), static_cast<float>(rw.value),
            static_cast<float>(rh.value));
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("setLineWidth", 1, 1, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        auto lw = ctx.float64Arg(0);
        if (!lw.success) {
            return qjs::Result<qjs::Value>::fail(lw.error);
        }
        win->set_line_width(static_cast<float>(lw.value));
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("moveTo", 2, 2, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        auto x = ctx.float64Arg(0);
        auto y = ctx.float64Arg(1);
        if (!x.success || !y.success) {
            return qjs::Result<qjs::Value>::fail(!x.success ? x.error : y.error);
        }
        win->move_to(static_cast<float>(x.value), static_cast<float>(y.value));
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("lineTo", 2, 2, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        auto x = ctx.float64Arg(0);
        auto y = ctx.float64Arg(1);
        if (!x.success || !y.success) {
            return qjs::Result<qjs::Value>::fail(!x.success ? x.error : y.error);
        }
        win->line_to(static_cast<float>(x.value), static_cast<float>(y.value));
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("stroke", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* win = require_window(ctx);
        if (!win) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui: call init() first", {}, {}});
        }
        win->stroke_path();
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.funcDynamic("createApp", 1, 1, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto app = ctx.valueArg(0);
        if (!app.success) {
            return qjs::Result<qjs::Value>::fail(app.error);
        }
        if (!app.value.isObject()) {
            return qjs::Result<qjs::Value>::fail(
                qjs::ErrorInfo{"ui.createApp: expected object with update and render", {}, {}});
        }
        auto update = app.value.getProperty("update");
        auto render = app.value.getProperty("render");
        if (!update.success || !render.success || !update.value.isFunction() || !render.value.isFunction()) {
            return qjs::Result<qjs::Value>::fail(
                qjs::ErrorInfo{"ui.createApp: update and render must be functions", {}, {}});
        }
        if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
            auto stored = ctx.valueArg(0);
            if (stored.success) {
                inst->set_deferred_app(std::move(stored.value));
            }
        }
        return qjs::Result<qjs::Value>::ok(std::move(app.value));
    });

    m.funcDynamic("runApp", 1, 2, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui.runApp: no active runtime instance", {}, {}});
        }

        inst->clear_deferred_app();

        auto appArg = ctx.valueArg(0);
        if (!appArg.success) {
            return qjs::Result<qjs::Value>::fail(appArg.error);
        }

        qianjs::AppHost host;
        if (!host.load_from_object(std::move(appArg.value))) {
            return qjs::Result<qjs::Value>::fail(
                qjs::ErrorInfo{"ui.runApp: app object must provide update and render functions", {}, {}});
        }

        qianjs::FrameLoopOptions opts;
        std::string title;
        if (!parse_frame_loop_options(ctx, opts, title)) {
            host.release();
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui.runApp: invalid options object", {}, {}});
        }

        if (!inst->run_frame_loop(host, opts)) {
            host.release();
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"ui.runApp: frame loop failed", {}, {}});
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });
}
