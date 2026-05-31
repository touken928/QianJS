#pragma once

#include "platform/platform_canvas.h"

#include <SDL.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace qianjs::platform {

/** Owns all live canvases for the current runtime instance. */
class CanvasRegistry {
public:
    uint64_t create(int width, int height, const std::string& title);
    PlatformCanvas* get(uint64_t id);
    const PlatformCanvas* get(uint64_t id) const;
    void destroy(uint64_t id);
    void destroy_all();

    /** Drain SDL queue; route events to per-canvas buffers (multi-window safe). */
    void pump_events();
    bool quit_requested() const { return quit_requested_; }

    std::vector<SDL_Event> take_events(uint64_t canvas_id);

private:
    uint64_t find_canvas_id_for_event(const SDL_Event& e) const;

    uint64_t next_id_ = 1;
    std::unordered_map<uint64_t, std::unique_ptr<PlatformCanvas>> canvases_;
    std::unordered_map<uint64_t, std::vector<SDL_Event>> pending_events_;
    bool quit_requested_ = false;
};

} // namespace qianjs::platform
