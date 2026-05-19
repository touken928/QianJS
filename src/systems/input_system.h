#pragma once

#include <quickjs.h>

#include <SDL.h>

#include <cstdint>
#include <vector>

namespace qianjs::platform {
class PlatformWindow;
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
    bool poll(platform::PlatformWindow& win, std::vector<SDL_Event>& batch, bool& should_quit);
    JSValue build_input_object(JSContext* c, const std::vector<SDL_Event>& batch, const InputFrame& frame) const;
};

} // namespace qianjs::systems
