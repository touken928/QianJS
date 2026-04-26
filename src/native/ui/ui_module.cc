#include "native/ui/ui_module.h"

#include <js_engine.h>
#include <js_module.h>
#include <quickjs.h>

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace {

struct UiState {
    bool inited = false;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    int w = 0;
    int h = 0;
    double color[4] = { 1, 1, 1, 1 };
    double line_width = 1;
};

UiState g_ui;

std::vector<std::pair<double, double>> g_path;

void ui_destroy() {
    g_path.clear();
    if (g_ui.renderer) {
        SDL_DestroyRenderer(g_ui.renderer);
        g_ui.renderer = nullptr;
    }
    if (g_ui.window) {
        SDL_DestroyWindow(g_ui.window);
        g_ui.window = nullptr;
    }
    g_ui.inited = false;
    g_ui.w = 0;
    g_ui.h = 0;
}

bool ui_require_inited(JSContext* c) {
    if (!g_ui.inited)
        (void)JS_ThrowTypeError(c, "ui: call init() first");
    return g_ui.inited;
}

Uint8 f2u8(double x) {
    const int v = static_cast<int>(std::lround(x * 255.0));
    return static_cast<Uint8>(std::clamp(v, 0, 255));
}

void apply_draw_color() {
    SDL_SetRenderDrawColor(g_ui.renderer, f2u8(g_ui.color[0]), f2u8(g_ui.color[1]), f2u8(g_ui.color[2]),
        f2u8(g_ui.color[3]));
}

void draw_line_segment(double x0, double y0, double x1, double y1) {
    const int ix0 = static_cast<int>(std::lround(x0));
    const int iy0 = static_cast<int>(std::lround(y0));
    const int ix1 = static_cast<int>(std::lround(x1));
    const int iy1 = static_cast<int>(std::lround(y1));
    if (g_ui.line_width <= 1.01) {
        SDL_RenderDrawLine(g_ui.renderer, ix0, iy0, ix1, iy1);
        return;
    }
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double len = std::hypot(dx, dy);
    if (len < 1e-9) {
        SDL_RenderDrawLine(g_ui.renderer, ix0, iy0, ix1, iy1);
        return;
    }
    const double nx = (-dy / len) * 0.5 * g_ui.line_width;
    const double ny = (dx / len) * 0.5 * g_ui.line_width;
    const int steps = std::max(1, static_cast<int>(std::lround(g_ui.line_width)));
    for (int s = 0; s < steps; ++s) {
        const double t = (static_cast<double>(s) + 0.5) / static_cast<double>(steps) - 0.5;
        const double ox = nx * t * 2.0;
        const double oy = ny * t * 2.0;
        SDL_RenderDrawLine(g_ui.renderer, static_cast<int>(std::lround(x0 + ox)), static_cast<int>(std::lround(y0 + oy)),
            static_cast<int>(std::lround(x1 + ox)), static_cast<int>(std::lround(y1 + oy)));
    }
}

void ui_present_internal() {
    if (g_ui.renderer)
        SDL_RenderPresent(g_ui.renderer);
}

void sdl_collect_events(std::vector<SDL_Event>& out) {
    SDL_Event e;
    while (SDL_PollEvent(&e))
        out.push_back(e);
}

bool sdl_batch_quit_or_escape(const std::vector<SDL_Event>& v) {
    for (const SDL_Event& e : v) {
        if (e.type == SDL_QUIT)
            return true;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            return true;
    }
    return false;
}

static const char* mouse_button_name(Uint8 b) {
    switch (b) {
    case SDL_BUTTON_LEFT:
        return "left";
    case SDL_BUTTON_MIDDLE:
        return "middle";
    case SDL_BUTTON_RIGHT:
        return "right";
    case SDL_BUTTON_X1:
        return "x1";
    case SDL_BUTTON_X2:
        return "x2";
    default:
        return "unknown";
    }
}

/** Converts one SDL event to a JS object, or JS_NULL for types we skip. */
JSValue sdl_event_to_js(JSContext* c, const SDL_Event& e) {
    JSValue o = JS_NewObject(c);
    if (JS_IsException(o))
        return o;

    switch (e.type) {
    case SDL_QUIT:
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, "quit")) < 0)
            goto fail;
        return o;

    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        const char* t = e.type == SDL_KEYDOWN ? "keydown" : "keyup";
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, t)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "repeat", JS_NewBool(c, e.key.repeat != 0)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "scancode", JS_NewInt32(c, static_cast<int>(e.key.keysym.scancode))) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "sym", JS_NewInt32(c, static_cast<int>(e.key.keysym.sym))) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "mod", JS_NewInt32(c, static_cast<int>(e.key.keysym.mod))) < 0)
            goto fail;
        {
            const char* kn = SDL_GetKeyName(e.key.keysym.sym);
            if (!kn)
                kn = "";
            JSValue ks = JS_NewString(c, kn);
            if (JS_IsException(ks)) {
                JS_FreeValue(c, o);
                return JS_EXCEPTION;
            }
            if (JS_SetPropertyStr(c, o, "key", ks) < 0) {
                JS_FreeValue(c, ks);
                goto fail;
            }
        }
        return o;
    }

    case SDL_MOUSEMOTION:
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, "mousemove")) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "x", JS_NewInt32(c, e.motion.x)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "y", JS_NewInt32(c, e.motion.y)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "dx", JS_NewInt32(c, e.motion.xrel)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "dy", JS_NewInt32(c, e.motion.yrel)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "which", JS_NewInt32(c, static_cast<int>(e.motion.which))) < 0)
            goto fail;
        return o;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        const char* t = e.type == SDL_MOUSEBUTTONDOWN ? "mousedown" : "mouseup";
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, t)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "button", JS_NewString(c, mouse_button_name(e.button.button))) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "buttonId", JS_NewInt32(c, static_cast<int>(e.button.button))) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "x", JS_NewInt32(c, e.button.x)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "y", JS_NewInt32(c, e.button.y)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "clicks", JS_NewInt32(c, static_cast<int>(e.button.clicks))) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "which", JS_NewInt32(c, static_cast<int>(e.button.which))) < 0)
            goto fail;
        return o;
    }

    case SDL_MOUSEWHEEL:
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, "mousewheel")) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "x", JS_NewInt32(c, e.wheel.x)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "y", JS_NewInt32(c, e.wheel.y)) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "direction", JS_NewInt32(c, static_cast<int>(e.wheel.direction))) < 0)
            goto fail;
        if (JS_SetPropertyStr(c, o, "which", JS_NewInt32(c, static_cast<int>(e.wheel.which))) < 0)
            goto fail;
        return o;

    default:
        JS_FreeValue(c, o);
        return JS_NULL;
    }

fail:
    JS_FreeValue(c, o);
    return JS_EXCEPTION;
}

JSValue sdl_batch_to_js_events(JSContext* c, const std::vector<SDL_Event>& v) {
    JSValue arr = JS_NewArray(c);
    if (JS_IsException(arr))
        return arr;

    uint32_t idx = 0;
    for (const SDL_Event& e : v) {
        JSValue item = sdl_event_to_js(c, e);
        if (JS_IsException(item)) {
            JS_FreeValue(c, arr);
            return JS_EXCEPTION;
        }
        if (JS_IsNull(item))
            continue;
        if (JS_SetPropertyUint32(c, arr, idx, item) < 0) {
            JS_FreeValue(c, item);
            JS_FreeValue(c, arr);
            return JS_EXCEPTION;
        }
        ++idx;
    }
    return arr;
}

} // namespace

const char* UiPlugin::name() const {
    return "ui";
}

void UiPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    auto& m = root.module("ui");

    m.funcDynamic("init", 2, 3, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        int w = 0;
        int h = 0;
        if (JS_ToInt32(c, &w, argv[0]) != 0 || JS_ToInt32(c, &h, argv[1]) != 0)
            return JS_EXCEPTION;
        if (w <= 0 || h <= 0)
            return JS_ThrowRangeError(c, "ui.init: width and height must be positive");

        std::string title = "QianJS";
        if (argc >= 3 && !JS_IsUndefined(argv[2])) {
            JSValue ts = JS_ToString(c, argv[2]);
            if (JS_IsException(ts))
                return JS_EXCEPTION;
            size_t len = 0;
            const char* p = JS_ToCStringLen(c, &len, ts);
            JS_FreeValue(c, ts);
            if (!p)
                return JS_EXCEPTION;
            title.assign(p, len);
            JS_FreeCString(c, p);
        }

        ui_destroy();

        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
            return JS_ThrowInternalError(c, "SDL_InitSubSystem: %s", SDL_GetError());

        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");

        g_ui.window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
            SDL_WINDOW_SHOWN);
        if (!g_ui.window) {
            ui_destroy();
            return JS_ThrowInternalError(c, "SDL_CreateWindow: %s", SDL_GetError());
        }

        /* Prefer software first: headless/dummy drivers and CI avoid GPU / vsync stalls. */
        g_ui.renderer = SDL_CreateRenderer(g_ui.window, -1, SDL_RENDERER_SOFTWARE);
        if (!g_ui.renderer)
            g_ui.renderer = SDL_CreateRenderer(g_ui.window, -1, SDL_RENDERER_ACCELERATED);
        if (!g_ui.renderer) {
            ui_destroy();
            return JS_ThrowInternalError(c, "SDL_CreateRenderer: %s", SDL_GetError());
        }

        SDL_SetRenderDrawBlendMode(g_ui.renderer, SDL_BLENDMODE_BLEND);

        g_ui.w = w;
        g_ui.h = h;
        g_ui.inited = true;

        SDL_SetRenderDrawColor(g_ui.renderer, 0, 0, 0, 255);
        SDL_RenderClear(g_ui.renderer);
        SDL_RenderPresent(g_ui.renderer);

        return JS_UNDEFINED;
    });

    m.funcDynamic("close", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)c;
        (void)argc;
        (void)argv;
        ui_destroy();
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return JS_UNDEFINED;
    });

    m.funcDynamic("pollEvents", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        std::vector<SDL_Event> batch;
        sdl_collect_events(batch);
        return JS_NewBool(c, sdl_batch_quit_or_escape(batch));
    });

    m.funcDynamic("readEvents", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        std::vector<SDL_Event> batch;
        sdl_collect_events(batch);
        const bool quit = sdl_batch_quit_or_escape(batch);
        JSValue events_js = sdl_batch_to_js_events(c, batch);
        if (JS_IsException(events_js))
            return JS_EXCEPTION;
        JSValue ret = JS_NewObject(c);
        if (JS_IsException(ret)) {
            JS_FreeValue(c, events_js);
            return JS_EXCEPTION;
        }
        if (JS_SetPropertyStr(c, ret, "quit", JS_NewBool(c, quit)) < 0) {
            JS_FreeValue(c, events_js);
            JS_FreeValue(c, ret);
            return JS_EXCEPTION;
        }
        if (JS_SetPropertyStr(c, ret, "events", events_js) < 0) {
            JS_FreeValue(c, ret);
            return JS_EXCEPTION;
        }
        return ret;
    });

    m.funcDynamic("getMouseState", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        int x = 0;
        int y = 0;
        const Uint32 mask = SDL_GetMouseState(&x, &y);
        JSValue o = JS_NewObject(c);
        if (JS_IsException(o))
            return JS_EXCEPTION;
        if (JS_SetPropertyStr(c, o, "x", JS_NewInt32(c, x)) < 0) {
            JS_FreeValue(c, o);
            return JS_EXCEPTION;
        }
        if (JS_SetPropertyStr(c, o, "y", JS_NewInt32(c, y)) < 0) {
            JS_FreeValue(c, o);
            return JS_EXCEPTION;
        }
        JSValue b = JS_NewObject(c);
        if (JS_IsException(b)) {
            JS_FreeValue(c, o);
            return JS_EXCEPTION;
        }
        const auto down = [mask](int btn) { return (mask & SDL_BUTTON(btn)) != 0; };
        if (JS_SetPropertyStr(c, b, "left", JS_NewBool(c, down(SDL_BUTTON_LEFT))) < 0
            || JS_SetPropertyStr(c, b, "middle", JS_NewBool(c, down(SDL_BUTTON_MIDDLE))) < 0
            || JS_SetPropertyStr(c, b, "right", JS_NewBool(c, down(SDL_BUTTON_RIGHT))) < 0
            || JS_SetPropertyStr(c, b, "x1", JS_NewBool(c, down(SDL_BUTTON_X1))) < 0
            || JS_SetPropertyStr(c, b, "x2", JS_NewBool(c, down(SDL_BUTTON_X2))) < 0) {
            JS_FreeValue(c, b);
            JS_FreeValue(c, o);
            return JS_EXCEPTION;
        }
        if (JS_SetPropertyStr(c, o, "buttons", b) < 0) {
            JS_FreeValue(c, o);
            return JS_EXCEPTION;
        }
        return o;
    });

    m.funcDynamic("getModState", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        const SDL_Keymod mod = SDL_GetModState();
        JSValue o = JS_NewObject(c);
        if (JS_IsException(o))
            return JS_EXCEPTION;
        const auto put = [&](const char* name, SDL_Keymod flag) -> bool {
            return JS_SetPropertyStr(c, o, name, JS_NewBool(c, (mod & flag) != 0)) >= 0;
        };
        if (!put("shift", KMOD_SHIFT) || !put("ctrl", KMOD_CTRL) || !put("alt", KMOD_ALT) || !put("gui", KMOD_GUI)
            || !put("num", KMOD_NUM) || !put("caps", KMOD_CAPS) || !put("lshift", KMOD_LSHIFT)
            || !put("rshift", KMOD_RSHIFT) || !put("lctrl", KMOD_LCTRL) || !put("rctrl", KMOD_RCTRL)
            || !put("lalt", KMOD_LALT) || !put("ralt", KMOD_RALT) || !put("lgui", KMOD_LGUI)
            || !put("rgui", KMOD_RGUI)) {
            JS_FreeValue(c, o);
            return JS_EXCEPTION;
        }
        if (JS_SetPropertyStr(c, o, "raw", JS_NewInt32(c, static_cast<int>(mod))) < 0) {
            JS_FreeValue(c, o);
            return JS_EXCEPTION;
        }
        return o;
    });

    m.funcDynamic("isKeyDown", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        SDL_Keycode key = SDLK_UNKNOWN;
        if (JS_IsNumber(argv[0])) {
            int32_t v = 0;
            if (JS_ToInt32(c, &v, argv[0]) != 0)
                return JS_EXCEPTION;
            key = static_cast<SDL_Keycode>(v);
        } else {
            JSValue ts = JS_ToString(c, argv[0]);
            if (JS_IsException(ts))
                return JS_EXCEPTION;
            const char* p = JS_ToCString(c, ts);
            JS_FreeValue(c, ts);
            if (!p)
                return JS_EXCEPTION;
            key = SDL_GetKeyFromName(p);
            JS_FreeCString(c, p);
        }
        const SDL_Scancode sc = SDL_GetScancodeFromKey(key);
        const Uint8* st = SDL_GetKeyboardState(nullptr);
        if (sc == SDL_SCANCODE_UNKNOWN)
            return JS_NewBool(c, false);
        return JS_NewBool(c, st[sc] != 0);
    });

    m.funcDynamic("present", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        ui_present_internal();
        return JS_UNDEFINED;
    });

    m.funcDynamic("setSourceRGBA", 4, 4, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        double r = 0, g = 0, b = 0, a = 1;
        if (JS_ToFloat64(c, &r, argv[0]) != 0 || JS_ToFloat64(c, &g, argv[1]) != 0 || JS_ToFloat64(c, &b, argv[2]) != 0
            || JS_ToFloat64(c, &a, argv[3]) != 0)
            return JS_EXCEPTION;
        g_ui.color[0] = r;
        g_ui.color[1] = g;
        g_ui.color[2] = b;
        g_ui.color[3] = a;
        return JS_UNDEFINED;
    });

    m.funcDynamic("clear", 4, 4, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        double r = 0, g = 0, b = 0, a = 1;
        if (JS_ToFloat64(c, &r, argv[0]) != 0 || JS_ToFloat64(c, &g, argv[1]) != 0 || JS_ToFloat64(c, &b, argv[2]) != 0
            || JS_ToFloat64(c, &a, argv[3]) != 0)
            return JS_EXCEPTION;
        SDL_SetRenderDrawColor(g_ui.renderer, f2u8(r), f2u8(g), f2u8(b), f2u8(a));
        SDL_RenderClear(g_ui.renderer);
        apply_draw_color();
        return JS_UNDEFINED;
    });

    m.funcDynamic("fillRect", 4, 4, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        double x = 0, y = 0, rw = 0, rh = 0;
        if (JS_ToFloat64(c, &x, argv[0]) != 0 || JS_ToFloat64(c, &y, argv[1]) != 0 || JS_ToFloat64(c, &rw, argv[2]) != 0
            || JS_ToFloat64(c, &rh, argv[3]) != 0)
            return JS_EXCEPTION;
        apply_draw_color();
        SDL_Rect rc;
        rc.x = static_cast<int>(std::lround(x));
        rc.y = static_cast<int>(std::lround(y));
        rc.w = static_cast<int>(std::lround(rw));
        rc.h = static_cast<int>(std::lround(rh));
        SDL_RenderFillRect(g_ui.renderer, &rc);
        return JS_UNDEFINED;
    });

    m.funcDynamic("strokeRect", 4, 4, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        double x = 0, y = 0, rw = 0, rh = 0;
        if (JS_ToFloat64(c, &x, argv[0]) != 0 || JS_ToFloat64(c, &y, argv[1]) != 0 || JS_ToFloat64(c, &rw, argv[2]) != 0
            || JS_ToFloat64(c, &rh, argv[3]) != 0)
            return JS_EXCEPTION;
        apply_draw_color();
        SDL_Rect rc;
        rc.x = static_cast<int>(std::lround(x));
        rc.y = static_cast<int>(std::lround(y));
        rc.w = static_cast<int>(std::lround(rw));
        rc.h = static_cast<int>(std::lround(rh));
        if (g_ui.line_width <= 1.01) {
            SDL_RenderDrawRect(g_ui.renderer, &rc);
            return JS_UNDEFINED;
        }
        const int n = std::max(1, static_cast<int>(std::lround(g_ui.line_width)));
        for (int i = 0; i < n; ++i) {
            SDL_Rect o = rc;
            o.x -= i;
            o.y -= i;
            o.w += i * 2;
            o.h += i * 2;
            SDL_RenderDrawRect(g_ui.renderer, &o);
        }
        return JS_UNDEFINED;
    });

    m.funcDynamic("setLineWidth", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        double lw = 1;
        if (JS_ToFloat64(c, &lw, argv[0]) != 0)
            return JS_EXCEPTION;
        g_ui.line_width = std::max(1.0, lw);
        return JS_UNDEFINED;
    });

    m.funcDynamic("moveTo", 2, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        double x = 0, y = 0;
        if (JS_ToFloat64(c, &x, argv[0]) != 0 || JS_ToFloat64(c, &y, argv[1]) != 0)
            return JS_EXCEPTION;
        g_path.clear();
        g_path.emplace_back(x, y);
        return JS_UNDEFINED;
    });

    m.funcDynamic("lineTo", 2, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        double x = 0, y = 0;
        if (JS_ToFloat64(c, &x, argv[0]) != 0 || JS_ToFloat64(c, &y, argv[1]) != 0)
            return JS_EXCEPTION;
        g_path.emplace_back(x, y);
        return JS_UNDEFINED;
    });

    m.funcDynamic("stroke", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        apply_draw_color();
        if (g_path.size() < 2)
            return JS_UNDEFINED;
        for (size_t i = 1; i < g_path.size(); ++i) {
            const auto& a = g_path[i - 1];
            const auto& b = g_path[i];
            draw_line_segment(a.first, a.second, b.first, b.second);
        }
        g_path.clear();
        return JS_UNDEFINED;
    });

    m.funcDynamic("runLoop", 1, 2, [&engine](JSContext* c, int argc, JSValue* argv) -> JSValue {
        if (!ui_require_inited(c))
            return JS_EXCEPTION;
        if (!JS_IsFunction(c, argv[0]))
            return JS_ThrowTypeError(c, "ui.runLoop: callback must be a function");

        int64_t max_frames = -1;
        if (argc >= 2 && !JS_IsUndefined(argv[1])) {
            if (JS_ToInt64(c, &max_frames, argv[1]) != 0)
                return JS_EXCEPTION;
        }

        JSValue cb = JS_DupValue(c, argv[0]);
        bool running = true;
        int64_t frame = 0;

        while (running) {
            std::vector<SDL_Event> batch;
            sdl_collect_events(batch);
            if (sdl_batch_quit_or_escape(batch))
                running = false;

            JSValue events_js = sdl_batch_to_js_events(c, batch);
            if (JS_IsException(events_js)) {
                JS_FreeValue(c, cb);
                return JS_EXCEPTION;
            }

            JSValue arg = JS_NewObject(c);
            if (JS_IsException(arg)) {
                JS_FreeValue(c, events_js);
                JS_FreeValue(c, cb);
                return JS_EXCEPTION;
            }
            if (JS_SetPropertyStr(c, arg, "frame", JS_NewInt64(c, frame)) < 0
                || JS_SetPropertyStr(c, arg, "events", events_js) < 0) {
                JS_FreeValue(c, arg);
                JS_FreeValue(c, cb);
                return JS_EXCEPTION;
            }

            JSValue call_argv[1] = { arg };
            JSValue r = JS_Call(c, cb, JS_UNDEFINED, 1, call_argv);
            JS_FreeValue(c, arg);
            if (JS_IsException(r)) {
                JS_FreeValue(c, cb);
                return JS_EXCEPTION;
            }
            JS_FreeValue(c, r);

            ui_present_internal();
            engine.pumpMicrotasks();

            ++frame;
            if (max_frames > 0 && frame >= max_frames)
                running = false;
        }

        JS_FreeValue(c, cb);
        return JS_UNDEFINED;
    });
}
