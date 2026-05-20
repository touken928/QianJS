#include "native/ui/ui_module.h"

#include "platform/ui_access.h"
#include "runtime/app_host.h"
#include "runtime/instance.h"

#include <js_engine.h>
#include <js_module.h>
#include <quickjs.h>

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace {

using qianjs::platform::require_window;

bool read_opt_int(JSContext* c, JSValue opts, const char* key, int* out) {
    JSValue v = JS_GetPropertyStr(c, opts, key);
    if (JS_IsUndefined(v)) {
        return true;
    }
    int32_t n = 0;
    const int r = JS_ToInt32(c, &n, v);
    JS_FreeValue(c, v);
    if (r != 0) {
        return false;
    }
    *out = static_cast<int>(n);
    return true;
}

bool read_opt_i64(JSContext* c, JSValue opts, const char* key, int64_t* out) {
    JSValue v = JS_GetPropertyStr(c, opts, key);
    if (JS_IsUndefined(v)) {
        return true;
    }
    if (JS_ToInt64(c, out, v) != 0) {
        JS_FreeValue(c, v);
        return false;
    }
    JS_FreeValue(c, v);
    return true;
}

bool read_opt_double(JSContext* c, JSValue opts, const char* key, double* out) {
    JSValue v = JS_GetPropertyStr(c, opts, key);
    if (JS_IsUndefined(v)) {
        return true;
    }
    if (JS_ToFloat64(c, out, v) != 0) {
        JS_FreeValue(c, v);
        return false;
    }
    JS_FreeValue(c, v);
    return true;
}

bool read_opt_string(JSContext* c, JSValue opts, const char* key, std::string* out) {
    JSValue v = JS_GetPropertyStr(c, opts, key);
    if (JS_IsUndefined(v)) {
        return true;
    }
    const char* s = JS_ToCString(c, v);
    JS_FreeValue(c, v);
    if (!s) {
        return false;
    }
    *out = s;
    JS_FreeCString(c, s);
    return true;
}

bool parse_frame_loop_options(JSContext* c, int argc, JSValue* argv, qianjs::FrameLoopOptions& opts, std::string& title_storage) {
    if (argc < 2 || JS_IsUndefined(argv[1])) {
        return true;
    }
    if (!JS_IsObject(argv[1])) {
        return false;
    }
    if (!read_opt_int(c, argv[1], "width", &opts.width)) {
        return false;
    }
    if (!read_opt_int(c, argv[1], "height", &opts.height)) {
        return false;
    }
    if (!read_opt_i64(c, argv[1], "maxFrames", &opts.max_frames)) {
        return false;
    }
    if (!read_opt_double(c, argv[1], "fps", &opts.target_fps)) {
        return false;
    }
    if (!read_opt_double(c, argv[1], "fixedStep", &opts.fixed_dt)) {
        return false;
    }
    if (!read_opt_string(c, argv[1], "title", &title_storage)) {
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

void UiPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    (void)engine;
    auto& m = root.module("ui");

    m.funcDynamic("init", 2, 3, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        int w = 0;
        int h = 0;
        if (JS_ToInt32(c, &w, argv[0]) != 0 || JS_ToInt32(c, &h, argv[1]) != 0) {
            return JS_EXCEPTION;
        }
        if (w <= 0 || h <= 0) {
            return JS_ThrowRangeError(c, "ui.init: width and height must be positive");
        }

        std::string title = "QianJS";
        if (argc >= 3 && !JS_IsUndefined(argv[2])) {
            JSValue ts = JS_ToString(c, argv[2]);
            if (JS_IsException(ts)) {
                return JS_EXCEPTION;
            }
            size_t len = 0;
            const char* p = JS_ToCStringLen(c, &len, ts);
            JS_FreeValue(c, ts);
            if (!p) {
                return JS_EXCEPTION;
            }
            title.assign(p, len);
            JS_FreeCString(c, p);
        }

        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return JS_ThrowInternalError(c, "ui.init: no active runtime instance");
        }
        if (!inst->ensure_window().init(w, h, title)) {
            return JS_ThrowInternalError(c, "ui.init: SDL initialization failed");
        }
        return JS_UNDEFINED;
    });

    m.funcDynamic("close", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)c;
        (void)argc;
        (void)argv;
        if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
            inst->destroy_window();
        }
        return JS_UNDEFINED;
    });

    m.funcDynamic("pollEvents", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        std::vector<SDL_Event> batch;
        if (auto* win = require_window(c)) {
            win->poll_events(batch);
        } else {
            return JS_EXCEPTION;
        }
        return JS_NewBool(c, qianjs::platform::PlatformWindow::batch_quit_or_escape(batch));
    });

    m.funcDynamic("readEvents", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        std::vector<SDL_Event> batch;
        win->poll_events(batch);
        const bool quit = qianjs::platform::PlatformWindow::batch_quit_or_escape(batch);
        JSValue events_js = qianjs::platform::PlatformWindow::events_to_js(c, batch);
        if (JS_IsException(events_js)) {
            return JS_EXCEPTION;
        }
        JSValue ret = JS_NewObject(c);
        if (JS_IsException(ret)) {
            JS_FreeValue(c, events_js);
            return JS_EXCEPTION;
        }
        if (JS_SetPropertyStr(c, ret, "quit", JS_NewBool(c, quit)) < 0
            || JS_SetPropertyStr(c, ret, "events", events_js) < 0) {
            JS_FreeValue(c, ret);
            return JS_EXCEPTION;
        }
        return ret;
    });

    m.funcDynamic("getMouseState", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        auto* win = require_window(c);
        return win ? win->mouse_state_js(c) : JS_EXCEPTION;
    });

    m.funcDynamic("getModState", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        auto* win = require_window(c);
        return win ? win->mod_state_js(c) : JS_EXCEPTION;
    });

    m.funcDynamic("isKeyDown", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        auto* win = require_window(c);
        return win ? win->is_key_down_js(c, argv[0]) : JS_EXCEPTION;
    });

    m.funcDynamic("present", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        win->present();
        return JS_UNDEFINED;
    });

    m.funcDynamic("setSourceRGBA", 4, 4, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        double r = 0, g = 0, b = 0, a = 1;
        if (JS_ToFloat64(c, &r, argv[0]) != 0 || JS_ToFloat64(c, &g, argv[1]) != 0 || JS_ToFloat64(c, &b, argv[2]) != 0
            || JS_ToFloat64(c, &a, argv[3]) != 0) {
            return JS_EXCEPTION;
        }
        win->set_color(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a));
        return JS_UNDEFINED;
    });

    m.funcDynamic("clear", 4, 4, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        double r = 0, g = 0, b = 0, a = 1;
        if (JS_ToFloat64(c, &r, argv[0]) != 0 || JS_ToFloat64(c, &g, argv[1]) != 0 || JS_ToFloat64(c, &b, argv[2]) != 0
            || JS_ToFloat64(c, &a, argv[3]) != 0) {
            return JS_EXCEPTION;
        }
        win->clear_framebuffer(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a));
        return JS_UNDEFINED;
    });

    m.funcDynamic("fillRect", 4, 4, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        double x = 0, y = 0, rw = 0, rh = 0;
        if (JS_ToFloat64(c, &x, argv[0]) != 0 || JS_ToFloat64(c, &y, argv[1]) != 0 || JS_ToFloat64(c, &rw, argv[2]) != 0
            || JS_ToFloat64(c, &rh, argv[3]) != 0) {
            return JS_EXCEPTION;
        }
        win->fill_rect(static_cast<float>(x), static_cast<float>(y), static_cast<float>(rw), static_cast<float>(rh));
        return JS_UNDEFINED;
    });

    m.funcDynamic("strokeRect", 4, 4, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        double x = 0, y = 0, rw = 0, rh = 0;
        if (JS_ToFloat64(c, &x, argv[0]) != 0 || JS_ToFloat64(c, &y, argv[1]) != 0 || JS_ToFloat64(c, &rw, argv[2]) != 0
            || JS_ToFloat64(c, &rh, argv[3]) != 0) {
            return JS_EXCEPTION;
        }
        win->stroke_rect(static_cast<float>(x), static_cast<float>(y), static_cast<float>(rw), static_cast<float>(rh));
        return JS_UNDEFINED;
    });

    m.funcDynamic("setLineWidth", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        double lw = 1;
        if (JS_ToFloat64(c, &lw, argv[0]) != 0) {
            return JS_EXCEPTION;
        }
        win->set_line_width(static_cast<float>(lw));
        return JS_UNDEFINED;
    });

    m.funcDynamic("moveTo", 2, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        double x = 0, y = 0;
        if (JS_ToFloat64(c, &x, argv[0]) != 0 || JS_ToFloat64(c, &y, argv[1]) != 0) {
            return JS_EXCEPTION;
        }
        win->move_to(static_cast<float>(x), static_cast<float>(y));
        return JS_UNDEFINED;
    });

    m.funcDynamic("lineTo", 2, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        double x = 0, y = 0;
        if (JS_ToFloat64(c, &x, argv[0]) != 0 || JS_ToFloat64(c, &y, argv[1]) != 0) {
            return JS_EXCEPTION;
        }
        win->line_to(static_cast<float>(x), static_cast<float>(y));
        return JS_UNDEFINED;
    });

    m.funcDynamic("stroke", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        auto* win = require_window(c);
        if (!win) {
            return JS_EXCEPTION;
        }
        win->stroke_path();
        return JS_UNDEFINED;
    });

    // Optional game loop (manual loop: init → pollEvents/readEvents → update logic → draw → present).
    m.funcDynamic("createApp", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!JS_IsObject(argv[0])) {
            return JS_ThrowTypeError(c, "ui.createApp: expected object with update and render");
        }
        JSValue update = JS_GetPropertyStr(c, argv[0], "update");
        JSValue render = JS_GetPropertyStr(c, argv[0], "render");
        const bool ok = JS_IsFunction(c, update) && JS_IsFunction(c, render);
        JS_FreeValue(c, update);
        JS_FreeValue(c, render);
        if (!ok) {
            return JS_ThrowTypeError(c, "ui.createApp: update and render must be functions");
        }
        if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
            inst->set_deferred_app(c, argv[0]);
        }
        return JS_DupValue(c, argv[0]);
    });

    m.funcDynamic("runApp", 1, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return JS_ThrowInternalError(c, "ui.runApp: no active runtime instance");
        }

        inst->clear_deferred_app();

        qianjs::AppHost host;
        if (!host.load_from_object(c, argv[0])) {
            return JS_ThrowTypeError(c, "ui.runApp: app object must provide update and render functions");
        }

        qianjs::FrameLoopOptions opts;
        std::string title;
        if (!parse_frame_loop_options(c, argc, argv, opts, title)) {
            host.release(c);
            return JS_EXCEPTION;
        }

        if (!inst->run_frame_loop(host, opts)) {
            host.release(c);
            return JS_EXCEPTION;
        }
        return JS_UNDEFINED;
    });
}
