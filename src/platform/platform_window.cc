#include "platform/platform_window.h"

#include <cstdlib>

#include <algorithm>
#include <cmath>

namespace qianjs::platform {

namespace {

const char* mouse_button_name(Uint8 b) {
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

JSValue sdl_event_to_js(JSContext* c, const SDL_Event& e) {
    JSValue o = JS_NewObject(c);
    if (JS_IsException(o)) {
        return o;
    }

    switch (e.type) {
    case SDL_QUIT:
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, "quit")) < 0) {
            goto fail;
        }
        return o;

    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        const char* t = e.type == SDL_KEYDOWN ? "keydown" : "keyup";
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, t)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "repeat", JS_NewBool(c, e.key.repeat != 0)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "scancode", JS_NewInt32(c, static_cast<int>(e.key.keysym.scancode))) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "sym", JS_NewInt32(c, static_cast<int>(e.key.keysym.sym))) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "mod", JS_NewInt32(c, static_cast<int>(e.key.keysym.mod))) < 0) {
            goto fail;
        }
        {
            const char* kn = SDL_GetKeyName(e.key.keysym.sym);
            if (!kn) {
                kn = "";
            }
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
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, "mousemove")) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "x", JS_NewInt32(c, e.motion.x)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "y", JS_NewInt32(c, e.motion.y)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "dx", JS_NewInt32(c, e.motion.xrel)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "dy", JS_NewInt32(c, e.motion.yrel)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "which", JS_NewInt32(c, static_cast<int>(e.motion.which))) < 0) {
            goto fail;
        }
        return o;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        const char* t = e.type == SDL_MOUSEBUTTONDOWN ? "mousedown" : "mouseup";
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, t)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "button", JS_NewString(c, mouse_button_name(e.button.button))) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "buttonId", JS_NewInt32(c, static_cast<int>(e.button.button))) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "x", JS_NewInt32(c, e.button.x)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "y", JS_NewInt32(c, e.button.y)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "clicks", JS_NewInt32(c, static_cast<int>(e.button.clicks))) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "which", JS_NewInt32(c, static_cast<int>(e.button.which))) < 0) {
            goto fail;
        }
        return o;
    }

    case SDL_MOUSEWHEEL:
        if (JS_SetPropertyStr(c, o, "type", JS_NewString(c, "mousewheel")) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "x", JS_NewInt32(c, e.wheel.x)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "y", JS_NewInt32(c, e.wheel.y)) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "direction", JS_NewInt32(c, static_cast<int>(e.wheel.direction))) < 0) {
            goto fail;
        }
        if (JS_SetPropertyStr(c, o, "which", JS_NewInt32(c, static_cast<int>(e.wheel.which))) < 0) {
            goto fail;
        }
        return o;

    default:
        JS_FreeValue(c, o);
        return JS_NULL;
    }

fail:
    JS_FreeValue(c, o);
    return JS_EXCEPTION;
}

} // namespace

bool PlatformWindow::env_null_ui_enabled() {
    const char* v = std::getenv("QIANJS_NULL_UI");
    return v && v[0] != '\0' && v[0] != '0';
}

bool PlatformWindow::init(int w, int h, const std::string& title) {
    destroy();
    if (w <= 0 || h <= 0) {
        return false;
    }

    null_mode_ = env_null_ui_enabled();
    if (null_mode_) {
        w_ = w;
        h_ = h;
        inited_ = true;
        draws_.reset();
        return true;
    }

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");

    window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    if (!window_) {
        destroy();
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    }
    if (!renderer_) {
        destroy();
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    w_ = w;
    h_ = h;
    inited_ = true;
    draws_.reset();
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
    return true;
}

void PlatformWindow::destroy() {
    draws_.reset();
    if (null_mode_) {
        null_mode_ = false;
        inited_ = false;
        w_ = 0;
        h_ = 0;
        return;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (inited_) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
    inited_ = false;
    w_ = 0;
    h_ = 0;
}

void PlatformWindow::present() {
    if (null_mode_) {
        draws_.reset();
        return;
    }
    if (!renderer_) {
        return;
    }
    draws_.execute(renderer_);
    SDL_RenderPresent(renderer_);
}

void PlatformWindow::poll_events(std::vector<SDL_Event>& out) const {
    out.clear();
    if (null_mode_) {
        return;
    }
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        out.push_back(e);
    }
}

bool PlatformWindow::batch_quit_or_escape(const std::vector<SDL_Event>& events) {
    for (const SDL_Event& e : events) {
        if (e.type == SDL_QUIT) {
            return true;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            return true;
        }
    }
    return false;
}

JSValue PlatformWindow::events_to_js(JSContext* c, const std::vector<SDL_Event>& events) {
    JSValue arr = JS_NewArray(c);
    if (JS_IsException(arr)) {
        return arr;
    }

    uint32_t idx = 0;
    for (const SDL_Event& e : events) {
        JSValue item = sdl_event_to_js(c, e);
        if (JS_IsException(item)) {
            JS_FreeValue(c, arr);
            return JS_EXCEPTION;
        }
        if (JS_IsNull(item)) {
            continue;
        }
        if (JS_SetPropertyUint32(c, arr, idx, item) < 0) {
            JS_FreeValue(c, item);
            JS_FreeValue(c, arr);
            return JS_EXCEPTION;
        }
        ++idx;
    }
    return arr;
}

JSValue PlatformWindow::mouse_state_js(JSContext* c) const {
    int x = 0;
    int y = 0;
    Uint32 mask = 0;
    if (!null_mode_) {
        mask = SDL_GetMouseState(&x, &y);
    }
    JSValue o = JS_NewObject(c);
    if (JS_IsException(o)) {
        return JS_EXCEPTION;
    }
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
}

JSValue PlatformWindow::mod_state_js(JSContext* c) const {
    const SDL_Keymod mod = null_mode_ ? static_cast<SDL_Keymod>(0) : SDL_GetModState();
    JSValue o = JS_NewObject(c);
    if (JS_IsException(o)) {
        return JS_EXCEPTION;
    }
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
}

JSValue PlatformWindow::is_key_down_js(JSContext* c, JSValue key_arg) const {
    SDL_Keycode key = SDLK_UNKNOWN;
    if (JS_IsNumber(key_arg)) {
        int32_t v = 0;
        if (JS_ToInt32(c, &v, key_arg) != 0) {
            return JS_EXCEPTION;
        }
        key = static_cast<SDL_Keycode>(v);
    } else {
        JSValue ts = JS_ToString(c, key_arg);
        if (JS_IsException(ts)) {
            return JS_EXCEPTION;
        }
        const char* p = JS_ToCString(c, ts);
        JS_FreeValue(c, ts);
        if (!p) {
            return JS_EXCEPTION;
        }
        key = SDL_GetKeyFromName(p);
        JS_FreeCString(c, p);
    }
    if (null_mode_) {
        return JS_NewBool(c, false);
    }
    const SDL_Scancode sc = SDL_GetScancodeFromKey(key);
    const Uint8* st = SDL_GetKeyboardState(nullptr);
    if (sc == SDL_SCANCODE_UNKNOWN) {
        return JS_NewBool(c, false);
    }
    return JS_NewBool(c, st[sc] != 0);
}

void PlatformWindow::clear_framebuffer(float r, float g, float b, float a) {
    draws_.clear_framebuffer(r, g, b, a);
}

void PlatformWindow::set_color(float r, float g, float b, float a) {
    draws_.set_color(r, g, b, a);
}

void PlatformWindow::set_line_width(float w) {
    draws_.set_line_width(w);
}

void PlatformWindow::fill_rect(float x, float y, float w, float h) {
    draws_.fill_rect(x, y, w, h);
}

void PlatformWindow::stroke_rect(float x, float y, float w, float h) {
    draws_.stroke_rect(x, y, w, h);
}

void PlatformWindow::move_to(float x, float y) {
    draws_.move_to(x, y);
}

void PlatformWindow::line_to(float x, float y) {
    draws_.line_to(x, y);
}

void PlatformWindow::stroke_path() {
    draws_.stroke_path();
}

} // namespace qianjs::platform
