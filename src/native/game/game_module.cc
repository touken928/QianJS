#include "native/game/game_module.h"

#include "platform/canvas_access.h"
#include "platform/js_bridge.h"
#include "platform/platform_canvas.h"
#include "runtime/app_host.h"
#include "runtime/instance.h"

#include <qjs/engine.h>
#include <qjs/module.h>
#include <qjs/value.h>

#include <functional>
#include <optional>
#include <string>

namespace {

bool parse_run_options(const qjs::Value& opt_val, qianjs::FrameLoopOptions& opts, std::string& title_storage) {
    if (opt_val.isUndefined()) {
        return true;
    }
    if (!opt_val.isObject()) {
        return false;
    }

    auto read_opt_int = [&](const char* key, int* out) -> bool {
        auto v = opt_val.getProperty(key);
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
        auto v = opt_val.getProperty(key);
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
        auto v = opt_val.getProperty(key);
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
        auto v = opt_val.getProperty(key);
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

void GamePlugin::install(qjs::Context& ctx, qjs::Module& root) {
    qjs::Engine& eng = ctx.engine();
    auto& m = root.module("game");

    m.func("isKeyDown", std::function<qjs::Value(qjs::Value)>([&eng](qjs::Value key) -> qjs::Value {
        return qianjs::platform::is_key_down_value(
            eng, qianjs::platform::PlatformCanvas::env_null_ui_enabled(), key);
    }));

    m.func("run",
        std::function<qjs::Value(qjs::Value, qjs::Value, std::optional<qjs::Value>)>(
            [&eng](qjs::Value canvasArg, qjs::Value appArg, std::optional<qjs::Value> options) -> qjs::Value {
        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return eng.throwTypeError("game.run: no active runtime instance");
        }

        inst->clear_deferred_app();

        if (!appArg.isObject()) {
            return eng.throwTypeError("game.run: app must be an object with update and render");
        }

        qianjs::platform::PlatformCanvas* canvas = qianjs::canvas::require_canvas(eng, canvasArg);
        if (!canvas) {
            return eng.undefined();
        }

        qianjs::AppHost host;
        if (!host.load_from_object(std::move(appArg))) {
            return eng.throwTypeError("game.run: app must provide update and render functions");
        }

        qianjs::FrameLoopOptions opts;
        opts.width = canvas->width();
        opts.height = canvas->height();
        std::string title;
        if (options && !parse_run_options(*options, opts, title)) {
            host.release();
            return eng.throwTypeError("game.run: invalid options");
        }

        if (!inst->run_frame_loop(*canvas, host, opts)) {
            host.release();
            return eng.throwTypeError("game.run: frame loop failed");
        }
        return eng.undefined();
    }));
}
