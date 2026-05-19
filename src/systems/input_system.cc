#include "systems/input_system.h"

#include "platform/platform_window.h"

namespace qianjs::systems {

bool InputSystem::poll(platform::PlatformWindow& win, std::vector<SDL_Event>& batch, bool& should_quit) {
    win.poll_events(batch);
    should_quit = platform::PlatformWindow::batch_quit_or_escape(batch);
    return true;
}

JSValue InputSystem::build_input_object(JSContext* c, const std::vector<SDL_Event>& batch,
    const InputFrame& frame) const {
    JSValue events_js = platform::PlatformWindow::events_to_js(c, batch);
    if (JS_IsException(events_js)) {
        return JS_EXCEPTION;
    }

    JSValue input = JS_NewObject(c);
    if (JS_IsException(input)) {
        JS_FreeValue(c, events_js);
        return JS_EXCEPTION;
    }

    if (JS_SetPropertyStr(c, input, "frame", JS_NewInt64(c, frame.frame)) < 0
        || JS_SetPropertyStr(c, input, "events", events_js) < 0
        || JS_SetPropertyStr(c, input, "dt", JS_NewFloat64(c, frame.dt)) < 0
        || JS_SetPropertyStr(c, input, "alpha", JS_NewFloat64(c, frame.alpha)) < 0) {
        JS_FreeValue(c, input);
        return JS_EXCEPTION;
    }
    return input;
}

} // namespace qianjs::systems
