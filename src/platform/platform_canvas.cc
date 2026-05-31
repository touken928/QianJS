#include "platform/platform_canvas.h"

#include "platform/js_bridge.h"

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION 1
#include <OpenGL/gl3.h>
#else
#include <SDL_opengl.h>
#endif

#include "nanovg.h"

#define NANOVG_GL3 1
#include "nanovg_gl.h" // NOLINT: GL3 API only; implementation in nanovg_backend.cc

#include <cstdlib>

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
        return e.button.windowID;
    default:
        return 0;
    }
}

} // namespace

int PlatformCanvas::sdl_video_refcount_ = 0;

bool PlatformCanvas::env_null_ui_enabled() {
    const char* v = std::getenv("QIANJS_NULL_UI");
    return v && v[0] != '\0' && v[0] != '0';
}

bool PlatformCanvas::init(int w, int h, const std::string& title) {
    destroy();
    if (w <= 0 || h <= 0) {
        return false;
    }

    null_mode_ = env_null_ui_enabled();
    w_ = w;
    h_ = h;
    inited_ = true;
    draws_.reset();

    if (null_mode_) {
        return true;
    }

    if (sdl_video_refcount_ == 0) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            inited_ = false;
            return false;
        }
    }
    ++sdl_video_refcount_;

#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window_) {
        destroy();
        return false;
    }

    sdl_window_id_ = SDL_GetWindowID(window_);
    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        destroy();
        return false;
    }
    SDL_GL_MakeCurrent(window_, gl_context_);
    SDL_GL_SetSwapInterval(0);

    if (!ensure_nvg()) {
        destroy();
        return false;
    }

    return true;
}

bool PlatformCanvas::ensure_nvg() {
    if (nvg_) {
        return true;
    }
    nvg_ = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!nvg_) {
        return false;
    }

    const char* font_path = std::getenv("QIANJS_FONT");
#ifndef QIANJS_DEFAULT_FONT
#define QIANJS_DEFAULT_FONT ""
#endif
    if (!font_path || !font_path[0]) {
        font_path = QIANJS_DEFAULT_FONT;
    }
    if (font_path && font_path[0]) {
        font_face_ = nvgCreateFont(nvg_, "sans", font_path);
        if (font_face_ < 0) {
            font_face_ = -1;
        }
    }
    return true;
}

float PlatformCanvas::pixel_ratio() const {
    if (!window_) {
        return 1.0f;
    }
    int dw = 0;
    int dh = 0;
    SDL_GL_GetDrawableSize(window_, &dw, &dh);
    if (w_ <= 0) {
        return 1.0f;
    }
    return static_cast<float>(dw) / static_cast<float>(w_);
}

void PlatformCanvas::destroy() {
    draws_.reset();
    font_face_ = -1;
    if (nvg_) {
        nvgDeleteGL3(nvg_);
        nvg_ = nullptr;
    }
    if (gl_context_) {
        SDL_GL_DeleteContext(gl_context_);
        gl_context_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (!null_mode_ && inited_ && sdl_video_refcount_ > 0) {
        --sdl_video_refcount_;
        if (sdl_video_refcount_ == 0) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
        }
    }
    null_mode_ = false;
    inited_ = false;
    w_ = 0;
    h_ = 0;
    sdl_window_id_ = 0;
}

void PlatformCanvas::present() {
    if (!inited_) {
        return;
    }
    if (null_mode_) {
        draws_.reset();
        return;
    }
    if (!window_ || !gl_context_ || !nvg_) {
        return;
    }
    SDL_GL_MakeCurrent(window_, gl_context_);
    int dw = w_;
    int dh = h_;
    SDL_GL_GetDrawableSize(window_, &dw, &dh);
    const float ratio = pixel_ratio();
    draws_.execute(nvg_, font_face_, static_cast<float>(w_), static_cast<float>(h_), ratio);
    SDL_GL_SwapWindow(window_);
}

void PlatformCanvas::poll_events(std::vector<SDL_Event>& out) const {
    out.clear();
    if (null_mode_) {
        return;
    }
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            out.push_back(e);
            continue;
        }
        const Uint32 wid = event_window_id(e);
        if (wid != 0 && wid != sdl_window_id_) {
            continue;
        }
        out.push_back(e);
    }
}

bool PlatformCanvas::batch_quit_or_escape(const std::vector<SDL_Event>& events, Uint32 window_id) {
    for (const SDL_Event& e : events) {
        if (e.type == SDL_QUIT) {
            return true;
        }
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE &&
            e.window.windowID == window_id) {
            return true;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE &&
            (e.window.windowID == 0 || e.window.windowID == window_id)) {
            return true;
        }
    }
    return false;
}

qjs::Value PlatformCanvas::events_to_js(qjs::Engine& engine, const std::vector<SDL_Event>& events, Uint32 window_id) {
    return events_to_value(engine, events, window_id);
}

qjs::Value PlatformCanvas::mouse_state_js(qjs::Engine& engine) const {
    return mouse_state_value(engine, null_mode_, window_);
}

qjs::Value PlatformCanvas::mod_state_js(qjs::Engine& engine) const {
    return mod_state_value(engine, null_mode_);
}

qjs::Value PlatformCanvas::is_key_down_js(qjs::Engine& engine, const qjs::Value& key_arg) const {
    return is_key_down_value(engine, null_mode_, key_arg);
}

void PlatformCanvas::set_fill_style(const Color4& c, const std::string& css) {
    fill_style_ = c;
    fill_style_css_ = css;
    draws_.set_fill_style(c);
}

void PlatformCanvas::set_stroke_style(const Color4& c, const std::string& css) {
    stroke_style_ = c;
    stroke_style_css_ = css;
    draws_.set_stroke_style(c);
}

void PlatformCanvas::set_line_width(float w) {
    line_width_ = w;
    draws_.set_line_width(w);
}

void PlatformCanvas::set_font_size(float s) {
    font_size_ = s > 0 ? s : 16.0f;
}

} // namespace qianjs::platform
