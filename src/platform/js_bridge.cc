#include "platform/js_bridge.h"

#include <qjs/object.h>

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

} // namespace

qjs::Value sdl_event_to_value(qjs::Engine& engine, const SDL_Event& e) {
    switch (e.type) {
    case SDL_QUIT:
        return engine.object().set("type", "quit").build();
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        const char* t = e.type == SDL_KEYDOWN ? "keydown" : "keyup";
        const char* kn = SDL_GetKeyName(e.key.keysym.sym);
        if (!kn) {
            kn = "";
        }
        return engine.object()
            .set("type", t)
            .set("repeat", e.key.repeat != 0)
            .set("scancode", static_cast<int64_t>(e.key.keysym.scancode))
            .set("sym", static_cast<int64_t>(e.key.keysym.sym))
            .set("mod", static_cast<int64_t>(e.key.keysym.mod))
            .set("key", kn)
            .build();
    }
    case SDL_MOUSEMOTION:
        return engine.object()
            .set("type", "mousemove")
            .set("x", e.motion.x)
            .set("y", e.motion.y)
            .set("dx", e.motion.xrel)
            .set("dy", e.motion.yrel)
            .set("which", static_cast<int64_t>(e.motion.which))
            .build();
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        const char* t = e.type == SDL_MOUSEBUTTONDOWN ? "mousedown" : "mouseup";
        return engine.object()
            .set("type", t)
            .set("button", mouse_button_name(e.button.button))
            .set("buttonId", static_cast<int64_t>(e.button.button))
            .set("x", e.button.x)
            .set("y", e.button.y)
            .set("clicks", static_cast<int64_t>(e.button.clicks))
            .set("which", static_cast<int64_t>(e.button.which))
            .build();
    }
    case SDL_MOUSEWHEEL:
        return engine.object()
            .set("type", "mousewheel")
            .set("x", e.wheel.x)
            .set("y", e.wheel.y)
            .set("direction", static_cast<int64_t>(e.wheel.direction))
            .set("which", static_cast<int64_t>(e.wheel.which))
            .build();
    default:
        return engine.nullValue();
    }
}

qjs::Value events_to_value(qjs::Engine& engine, const std::vector<SDL_Event>& events, Uint32 window_id) {
    auto arr = engine.array();
    for (const SDL_Event& e : events) {
        if (window_id != 0 && e.type != SDL_QUIT) {
            if (e.type == SDL_WINDOWEVENT && e.window.windowID != window_id) {
                continue;
            }
            if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP || e.type == SDL_MOUSEMOTION ||
                    e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEWHEEL) &&
                e.window.windowID != 0 && e.window.windowID != window_id) {
                continue;
            }
        }
        qjs::Value item = sdl_event_to_value(engine, e);
        if (item.isNull()) {
            continue;
        }
        arr.push(std::move(item));
    }
    return arr.build();
}

qjs::Value mouse_state_value(qjs::Engine& engine, bool null_mode, SDL_Window* window) {
    int x = 0;
    int y = 0;
    Uint32 mask = 0;
    if (!null_mode) {
        if (window && SDL_GetMouseFocus() != window) {
            mask = 0;
        } else {
            mask = SDL_GetMouseState(&x, &y);
            if (window) {
                int wx = 0;
                int wy = 0;
                SDL_GetWindowPosition(window, &wx, &wy);
                x -= wx;
                y -= wy;
            }
        }
    }
    const auto down = [mask](int btn) { return (mask & SDL_BUTTON(btn)) != 0; };
    qjs::Value buttons = engine.object()
                            .set("left", down(SDL_BUTTON_LEFT))
                            .set("middle", down(SDL_BUTTON_MIDDLE))
                            .set("right", down(SDL_BUTTON_RIGHT))
                            .set("x1", down(SDL_BUTTON_X1))
                            .set("x2", down(SDL_BUTTON_X2))
                            .build();
    return engine.object().set("x", x).set("y", y).set("buttons", std::move(buttons)).build();
}

qjs::Value mod_state_value(qjs::Engine& engine, bool null_mode) {
    const SDL_Keymod mod = null_mode ? static_cast<SDL_Keymod>(0) : SDL_GetModState();
    const auto f = [&](SDL_Keymod flag) { return (mod & flag) != 0; };
    return engine.object()
        .set("shift", f(KMOD_SHIFT))
        .set("ctrl", f(KMOD_CTRL))
        .set("alt", f(KMOD_ALT))
        .set("gui", f(KMOD_GUI))
        .set("num", f(KMOD_NUM))
        .set("caps", f(KMOD_CAPS))
        .set("lshift", f(KMOD_LSHIFT))
        .set("rshift", f(KMOD_RSHIFT))
        .set("lctrl", f(KMOD_LCTRL))
        .set("rctrl", f(KMOD_RCTRL))
        .set("lalt", f(KMOD_LALT))
        .set("ralt", f(KMOD_RALT))
        .set("lgui", f(KMOD_LGUI))
        .set("rgui", f(KMOD_RGUI))
        .set("raw", static_cast<int64_t>(mod))
        .build();
}

qjs::Value is_key_down_value(qjs::Engine& engine, bool null_mode, const qjs::Value& key_arg) {
    if (null_mode) {
        return engine.boolValue(false);
    }
    SDL_Keycode key = SDLK_UNKNOWN;
    if (auto n = key_arg.toInt32(); n.success) {
        key = static_cast<SDL_Keycode>(n.value);
    } else if (auto s = key_arg.toString(); s.success) {
        key = SDL_GetKeyFromName(s.value.c_str());
    }
    const SDL_Scancode sc = SDL_GetScancodeFromKey(key);
    const Uint8* st = SDL_GetKeyboardState(nullptr);
    if (sc == SDL_SCANCODE_UNKNOWN) {
        return engine.boolValue(false);
    }
    return engine.boolValue(st[sc] != 0);
}

} // namespace qianjs::platform
