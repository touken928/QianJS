#pragma once

#include <qjs/engine.h>
#include <qjs/value.h>

namespace qianjs::platform {
class PlatformCanvas;
}

namespace qianjs::systems {

class RenderSystem {
public:
    bool render_frame(platform::PlatformCanvas& canvas, qjs::Engine& engine, const qjs::Value& render_fn);
};

} // namespace qianjs::systems
