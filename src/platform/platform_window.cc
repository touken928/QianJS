#include "platform/platform_window.h"

#include "platform/js_bridge.h"

#include <cstdlib>

#include <algorithm>
#include <cmath>

namespace qianjs::platform {

bool PlatformWindow::env_null_ui_enabled() {
    const char* v = std::getenv("QIANJS_NULL_UI");
    return v && v[0] != '\0' && v[0] != '0';
}

bool PlatformWindow::init(int w, int h, const std::string& title) {
    destroy();
    if (w <= 0 || h <= 0) {
        return false;
    }

    null_mode_ = env_null_ui_enabled();
    if (null_mode_) {
        w_ = w;
        h_ = h;
        inited_ = true;
        draws_.reset();
        return true;
    }

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");

    window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    if (!window_) {
        destroy();
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    }
    if (!renderer_) {
        destroy();
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    w_ = w;
    h_ = h;
    inited_ = true;
    draws_.reset();
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
    return true;
}

void PlatformWindow::destroy() {
    draws_.reset();
    if (null_mode_) {
        null_mode_ = false;
        inited_ = false;
        w_ = 0;
        h_ = 0;
        return;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (inited_) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
    inited_ = false;
    w_ = 0;
    h_ = 0;
}

void PlatformWindow::present() {
    if (null_mode_) {
        draws_.reset();
        return;
    }
    if (!renderer_) {
        return;
    }
    draws_.execute(renderer_);
    SDL_RenderPresent(renderer_);
}

void PlatformWindow::poll_events(std::vector<SDL_Event>& out) const {
    out.clear();
    if (null_mode_) {
        return;
    }
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        out.push_back(e);
    }
}

bool PlatformWindow::batch_quit_or_escape(const std::vector<SDL_Event>& events) {
    for (const SDL_Event& e : events) {
        if (e.type == SDL_QUIT) {
            return true;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            return true;
        }
    }
    return false;
}

qjs::Value PlatformWindow::events_to_js(qjs::Engine& engine, const std::vector<SDL_Event>& events) {
    return events_to_value(engine, events);
}

qjs::Value PlatformWindow::mouse_state_js(qjs::Engine& engine) const {
    return mouse_state_value(engine, null_mode_);
}

qjs::Value PlatformWindow::mod_state_js(qjs::Engine& engine) const {
    return mod_state_value(engine, null_mode_);
}

qjs::Value PlatformWindow::is_key_down_js(qjs::Engine& engine, const qjs::Value& key_arg) const {
    return is_key_down_value(engine, null_mode_, key_arg);
}

void PlatformWindow::clear_framebuffer(float r, float g, float b, float a) {
    draws_.clear_framebuffer(r, g, b, a);
}

void PlatformWindow::set_color(float r, float g, float b, float a) {
    draws_.set_color(r, g, b, a);
}

void PlatformWindow::set_line_width(float w) {
    draws_.set_line_width(w);
}

void PlatformWindow::fill_rect(float x, float y, float w, float h) {
    draws_.fill_rect(x, y, w, h);
}

void PlatformWindow::stroke_rect(float x, float y, float w, float h) {
    draws_.stroke_rect(x, y, w, h);
}

void PlatformWindow::move_to(float x, float y) {
    draws_.move_to(x, y);
}

void PlatformWindow::line_to(float x, float y) {
    draws_.line_to(x, y);
}

void PlatformWindow::stroke_path() {
    draws_.stroke_path();
}

} // namespace qianjs::platform
