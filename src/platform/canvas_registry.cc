#include "platform/canvas_registry.h"

namespace qianjs::platform {

namespace {

Uint32 event_window_id(const SDL_Event& e) {
    switch (e.type) {
    case SDL_WINDOWEVENT:
        return e.window.windowID;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        return e.key.windowID;
    case SDL_MOUSEMOTION:
        return e.motion.windowID;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        return e.button.windowID;
    case SDL_MOUSEWHEEL:
        return e.wheel.windowID;
    default:
        return 0;
    }
}

} // namespace

uint64_t CanvasRegistry::create(int width, int height, const std::string& title) {
    const uint64_t id = next_id_++;
    auto canvas = std::make_unique<PlatformCanvas>();
    if (!canvas->init(width, height, title)) {
        return 0;
    }
    canvases_[id] = std::move(canvas);
    return id;
}

PlatformCanvas* CanvasRegistry::get(uint64_t id) {
    auto it = canvases_.find(id);
    return it == canvases_.end() ? nullptr : it->second.get();
}

const PlatformCanvas* CanvasRegistry::get(uint64_t id) const {
    auto it = canvases_.find(id);
    return it == canvases_.end() ? nullptr : it->second.get();
}

void CanvasRegistry::destroy(uint64_t id) {
    pending_events_.erase(id);
    canvases_.erase(id);
}

void CanvasRegistry::destroy_all() {
    pending_events_.clear();
    quit_requested_ = false;
    canvases_.clear();
}

uint64_t CanvasRegistry::find_canvas_id_for_event(const SDL_Event& e) const {
    if (e.type == SDL_QUIT) {
        return 0;
    }
    const Uint32 wid = event_window_id(e);
    if (wid == 0) {
        return 0;
    }
    for (const auto& [id, canvas] : canvases_) {
        if (canvas->sdl_window_id() == wid) {
            return id;
        }
    }
    return 0;
}

void CanvasRegistry::pump_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit_requested_ = true;
            continue;
        }
        const uint64_t id = find_canvas_id_for_event(e);
        if (id != 0) {
            pending_events_[id].push_back(e);
        }
    }
}

std::vector<SDL_Event> CanvasRegistry::take_events(uint64_t canvas_id) {
    auto it = pending_events_.find(canvas_id);
    if (it == pending_events_.end()) {
        return {};
    }
    std::vector<SDL_Event> out = std::move(it->second);
    pending_events_.erase(it);
    return out;
}

} // namespace qianjs::platform
