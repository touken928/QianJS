#include "systems/input_system.h"

#include "platform/platform_canvas.h"

namespace qianjs::systems {

bool InputSystem::poll(platform::PlatformCanvas& canvas, std::vector<SDL_Event>& batch, bool& should_quit) {
    canvas.poll_events(batch);
    should_quit = platform::PlatformCanvas::batch_quit_or_escape(batch, canvas.sdl_window_id());
    return true;
}

qjs::Value InputSystem::build_input_object(qjs::Engine& engine, platform::PlatformCanvas& canvas,
    const std::vector<SDL_Event>& batch, const InputFrame& frame) const {
    return engine.object()
        .setInt64("frame", frame.frame)
        .setDouble("dt", frame.dt)
        .setDouble("alpha", frame.alpha)
        .set("events", platform::PlatformCanvas::events_to_js(engine, batch, canvas.sdl_window_id()))
        .set("mouse", canvas.mouse_state_js(engine))
        .set("mods", canvas.mod_state_js(engine))
        .build();
}

} // namespace qianjs::systems
