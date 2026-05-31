#include "systems/render_system.h"

#include "platform/platform_canvas.h"

namespace qianjs::systems {

bool RenderSystem::render_frame(platform::PlatformCanvas& canvas, qjs::Engine& engine, const qjs::Value& render_fn) {
    canvas.draw_list().reset();
    if (!engine.call(render_fn).success) {
        return false;
    }
    canvas.present();
    return true;
}

} // namespace qianjs::systems
