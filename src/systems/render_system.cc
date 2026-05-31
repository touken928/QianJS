#include "systems/render_system.h"

#include "platform/platform_window.h"

namespace qianjs::systems {

bool RenderSystem::render_frame(platform::PlatformWindow& win, qjs::Engine& engine, const qjs::Value& render_fn) {
    win.draws().reset();
    if (!engine.call(render_fn).success) {
        return false;
    }
    win.present();
    return true;
}

} // namespace qianjs::systems
