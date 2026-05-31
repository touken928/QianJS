#pragma once

#include <qjs/engine.h>
#include <qjs/value.h>

#include <SDL.h>

#include <cstdint>
#include <vector>

namespace qianjs::platform {
class PlatformCanvas;
}

namespace qianjs::systems {

struct InputFrame {
    int64_t frame = 0;
    double dt = 0;
    double alpha = 0;
};

/** Poll SDL (or null platform) and build the `input` object for `update(dt, input)`. */
class InputSystem {
public:
    bool poll(platform::PlatformCanvas& canvas, std::vector<SDL_Event>& batch, bool& should_quit);
    qjs::Value build_input_object(qjs::Engine& engine, platform::PlatformCanvas& canvas,
        const std::vector<SDL_Event>& batch, const InputFrame& frame) const;
};

} // namespace qianjs::systems
