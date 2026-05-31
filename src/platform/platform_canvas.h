#pragma once

#include "platform/draw_list.h"

#include <qjs/engine.h>
#include <qjs/value.h>

#include <SDL.h>

#include <string>
#include <vector>

struct NVGcontext;

namespace qianjs::platform {

/** One SDL window + NanoVG context + retained draw list. */
class PlatformCanvas {
public:
    bool inited() const { return inited_; }
    int width() const { return w_; }
    int height() const { return h_; }
    Uint32 sdl_window_id() const { return sdl_window_id_; }

    bool init(int w, int h, const std::string& title);
    void destroy();

    bool null_mode() const { return null_mode_; }
    static bool env_null_ui_enabled();

    DrawList& draws() { return draws_; }
    void present();

    void poll_events(std::vector<SDL_Event>& out) const;
    static bool batch_quit_or_escape(const std::vector<SDL_Event>& events, Uint32 window_id);
    static qjs::Value events_to_js(qjs::Engine& engine, const std::vector<SDL_Event>& events, Uint32 window_id);

    qjs::Value mouse_state_js(qjs::Engine& engine) const;
    qjs::Value mod_state_js(qjs::Engine& engine) const;
    qjs::Value is_key_down_js(qjs::Engine& engine, const qjs::Value& key_arg) const;

    DrawList& draw_list() { return draws_; }

    void set_fill_style(const Color4& c, const std::string& css);
    void set_stroke_style(const Color4& c, const std::string& css);
    const Color4& fill_style() const { return fill_style_; }
    const Color4& stroke_style() const { return stroke_style_; }
    const std::string& fill_style_css() const { return fill_style_css_; }
    const std::string& stroke_style_css() const { return stroke_style_css_; }
    float line_width() const { return line_width_; }
    void set_line_width(float w);
    float font_size() const { return font_size_; }
    void set_font_size(float s);

private:
    bool ensure_nvg();
    float pixel_ratio() const;

    static int sdl_video_refcount_;

    bool inited_ = false;
    bool null_mode_ = false;
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    NVGcontext* nvg_ = nullptr;
    int font_face_ = -1;
    int w_ = 0;
    int h_ = 0;
    Uint32 sdl_window_id_ = 0;
    DrawList draws_;
    Color4 fill_style_{0, 0, 0, 1};
    Color4 stroke_style_{0, 0, 0, 1};
    std::string fill_style_css_ = "#000000";
    std::string stroke_style_css_ = "#000000";
    float line_width_ = 1.0f;
    float font_size_ = 16.0f;
};

} // namespace qianjs::platform
