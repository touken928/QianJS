#include "systems/render_system.h"

#include "platform/platform_window.h"

namespace qianjs::systems {

bool RenderSystem::render_frame(platform::PlatformWindow& win, JSContext* c, JSValue render_fn) {
    win.draws().reset();
    JSValue r = JS_Call(c, render_fn, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(r)) {
        return false;
    }
    JS_FreeValue(c, r);
    win.present();
    return true;
}

} // namespace qianjs::systems
