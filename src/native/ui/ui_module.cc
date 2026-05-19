#include "native/ui/ui_module.h"

#include "platform/ui_access.h"
#include "runtime/instance.h"

#include <js_engine.h>
#include <js_module.h>
#include <quickjs.h>

#include <cmath>
#include <string>
#include <vector>

namespace {

using qianjs::platform::require_window;

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
}
