#include "systems/input_system.h"

#include "platform/platform_window.h"

#include <qjs/object.h>

namespace qianjs::systems {

bool InputSystem::poll(platform::PlatformWindow& win, std::vector<SDL_Event>& batch, bool& should_quit) {
    win.poll_events(batch);
    should_quit = platform::PlatformWindow::batch_quit_or_escape(batch);
    return true;
}

qjs::Value InputSystem::build_input_object(qjs::Engine& engine, const std::vector<SDL_Event>& batch,
    const InputFrame& frame) const {
    qjs::Value events_js = platform::PlatformWindow::events_to_js(engine, batch);
    return engine.object()
        .setInt64("frame", frame.frame)
        .set("events", std::move(events_js))
        .setDouble("dt", frame.dt)
        .setDouble("alpha", frame.alpha)
        .build();
}

} // namespace qianjs::systems
