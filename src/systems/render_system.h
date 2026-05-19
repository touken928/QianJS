#pragma once

#include <quickjs.h>

namespace qianjs::platform {
class PlatformWindow;
}

namespace qianjs::systems {

/** Runs `render()` and presents the deferred DrawList. */
class RenderSystem {
public:
    bool render_frame(platform::PlatformWindow& win, JSContext* c, JSValue render_fn);
};

} // namespace qianjs::systems
