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
        return engine.object().setString("type", "quit").build();
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        const char* t = e.type == SDL_KEYDOWN ? "keydown" : "keyup";
        const char* kn = SDL_GetKeyName(e.key.keysym.sym);
        if (!kn) {
            kn = "";
        }
        return engine.object()
            .setString("type", t)
            .setBool("repeat", e.key.repeat != 0)
            .setInt64("scancode", static_cast<int64_t>(e.key.keysym.scancode))
            .setInt64("sym", static_cast<int64_t>(e.key.keysym.sym))
            .setInt64("mod", static_cast<int64_t>(e.key.keysym.mod))
            .setString("key", kn)
            .build();
    }
    case SDL_MOUSEMOTION:
        return engine.object()
            .setString("type", "mousemove")
            .setInt64("x", e.motion.x)
            .setInt64("y", e.motion.y)
            .setInt64("dx", e.motion.xrel)
            .setInt64("dy", e.motion.yrel)
            .setInt64("which", static_cast<int64_t>(e.motion.which))
            .build();
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        const char* t = e.type == SDL_MOUSEBUTTONDOWN ? "mousedown" : "mouseup";
        return engine.object()
            .setString("type", t)
            .setString("button", mouse_button_name(e.button.button))
            .setInt64("buttonId", static_cast<int64_t>(e.button.button))
            .setInt64("x", e.button.x)
            .setInt64("y", e.button.y)
            .setInt64("clicks", static_cast<int64_t>(e.button.clicks))
            .setInt64("which", static_cast<int64_t>(e.button.which))
            .build();
    }
    case SDL_MOUSEWHEEL:
        return engine.object()
            .setString("type", "mousewheel")
            .setInt64("x", e.wheel.x)
            .setInt64("y", e.wheel.y)
            .setInt64("direction", static_cast<int64_t>(e.wheel.direction))
            .setInt64("which", static_cast<int64_t>(e.wheel.which))
            .build();
    default:
        return engine.nullValue();
    }
}

qjs::Value events_to_value(qjs::Engine& engine, const std::vector<SDL_Event>& events) {
    auto arr = engine.array();
    for (const SDL_Event& e : events) {
        qjs::Value item = sdl_event_to_value(engine, e);
        if (item.isNull()) {
            continue;
        }
        arr.push(std::move(item));
    }
    return arr.build();
}

qjs::Value mouse_state_value(qjs::Engine& engine, bool null_mode) {
    int x = 0;
    int y = 0;
    Uint32 mask = 0;
    if (!null_mode) {
        mask = SDL_GetMouseState(&x, &y);
    }
    const auto down = [mask](int btn) { return (mask & SDL_BUTTON(btn)) != 0; };
    qjs::Value buttons = engine.object()
                            .setBool("left", down(SDL_BUTTON_LEFT))
                            .setBool("middle", down(SDL_BUTTON_MIDDLE))
                            .setBool("right", down(SDL_BUTTON_RIGHT))
                            .setBool("x1", down(SDL_BUTTON_X1))
                            .setBool("x2", down(SDL_BUTTON_X2))
                            .build();
    return engine.object().setInt64("x", x).setInt64("y", y).set("buttons", std::move(buttons)).build();
}

qjs::Value mod_state_value(qjs::Engine& engine, bool null_mode) {
    const SDL_Keymod mod = null_mode ? static_cast<SDL_Keymod>(0) : SDL_GetModState();
    const auto f = [&](SDL_Keymod flag) { return (mod & flag) != 0; };
    return engine.object()
        .setBool("shift", f(KMOD_SHIFT))
        .setBool("ctrl", f(KMOD_CTRL))
        .setBool("alt", f(KMOD_ALT))
        .setBool("gui", f(KMOD_GUI))
        .setBool("num", f(KMOD_NUM))
        .setBool("caps", f(KMOD_CAPS))
        .setBool("lshift", f(KMOD_LSHIFT))
        .setBool("rshift", f(KMOD_RSHIFT))
        .setBool("lctrl", f(KMOD_LCTRL))
        .setBool("rctrl", f(KMOD_RCTRL))
        .setBool("lalt", f(KMOD_LALT))
        .setBool("ralt", f(KMOD_RALT))
        .setBool("lgui", f(KMOD_LGUI))
        .setBool("rgui", f(KMOD_RGUI))
        .setInt64("raw", static_cast<int64_t>(mod))
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
