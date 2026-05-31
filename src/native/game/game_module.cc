#include "native/game/game_module.h"

#include "platform/canvas_access.h"
#include "platform/js_bridge.h"
#include "platform/platform_canvas.h"
#include "runtime/app_host.h"
#include "runtime/instance.h"

#include <qjs/call.h>
#include <qjs/engine.h>
#include <qjs/module.h>

#include <string>

namespace {

bool parse_run_options(qjs::CallContext& ctx, int options_index, qianjs::FrameLoopOptions& opts,
    std::string& title_storage) {
    if (ctx.argc() <= options_index) {
        return true;
    }
    auto optVal = ctx.valueArg(options_index);
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

const char* GamePlugin::name() const {
    return "game";
}

void GamePlugin::install(qjs::Context&, qjs::Module& root) {
    auto& m = root.module("game");

    m.funcDynamic("isKeyDown", 1, 1, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto key = ctx.valueArg(0);
        if (!key.success) {
            return qjs::Result<qjs::Value>::fail(key.error);
        }
        return qjs::Result<qjs::Value>::ok(qianjs::platform::is_key_down_value(
            ctx.engine(), qianjs::platform::PlatformCanvas::env_null_ui_enabled(), key.value));
    });

    m.funcDynamic("run", 2, 3, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"game.run: no active runtime instance", {}, {}});
        }

        inst->clear_deferred_app();

        auto canvasArg = ctx.valueArg(0);
        auto appArg = ctx.valueArg(1);
        if (!canvasArg.success || !appArg.success) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"game.run: expected (canvas, app)", {}, {}});
        }
        if (!appArg.value.isObject()) {
            return qjs::Result<qjs::Value>::fail(
                qjs::ErrorInfo{"game.run: app must be an object with update and render", {}, {}});
        }

        qianjs::platform::PlatformCanvas* canvas = qianjs::canvas::require_canvas(ctx, canvasArg.value);
        if (!canvas) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"game.run: invalid canvas", {}, {}});
        }

        qianjs::AppHost host;
        if (!host.load_from_object(std::move(appArg.value))) {
            return qjs::Result<qjs::Value>::fail(
                qjs::ErrorInfo{"game.run: app must provide update and render functions", {}, {}});
        }

        qianjs::FrameLoopOptions opts;
        opts.width = canvas->width();
        opts.height = canvas->height();
        std::string title;
        if (!parse_run_options(ctx, 2, opts, title)) {
            host.release();
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"game.run: invalid options", {}, {}});
        }

        if (!inst->run_frame_loop(*canvas, host, opts)) {
            host.release();
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"game.run: frame loop failed", {}, {}});
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });
}
