#pragma once

#include <qjs/engine.h>
#include <qjs/value.h>

namespace qianjs::platform {
class PlatformWindow;
}

namespace qianjs::systems {

/** Runs `render()` and presents the deferred DrawList. */
class RenderSystem {
public:
    bool render_frame(platform::PlatformWindow& win, qjs::Engine& engine, const qjs::Value& render_fn);
};

} // namespace qianjs::systems
