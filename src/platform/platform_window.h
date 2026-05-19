#pragma once

#include "platform/draw_list.h"

#include <quickjs.h>

#include <SDL.h>

#include <string>
#include <vector>

namespace qianjs::platform {

/** Per-runtime SDL window + deferred draw list (owned by RuntimeInstance). */
class PlatformWindow {
public:
    bool inited() const { return inited_; }
    int width() const { return w_; }
    int height() const { return h_; }

    bool init(int w, int h, const std::string& title);
    void destroy();

    /** Headless mode: no SDL window (env `QIANJS_NULL_UI=1`). DrawList still records commands. */
    bool null_mode() const { return null_mode_; }
    static bool env_null_ui_enabled();

    DrawList& draws() { return draws_; }
    void present();

    void poll_events(std::vector<SDL_Event>& out) const;
    static bool batch_quit_or_escape(const std::vector<SDL_Event>& events);
    static JSValue events_to_js(JSContext* c, const std::vector<SDL_Event>& events);

    JSValue mouse_state_js(JSContext* c) const;
    JSValue mod_state_js(JSContext* c) const;
    JSValue is_key_down_js(JSContext* c, JSValue key_arg) const;

    void clear_framebuffer(float r, float g, float b, float a);
    void set_color(float r, float g, float b, float a);
    void set_line_width(float w);
    void fill_rect(float x, float y, float w, float h);
    void stroke_rect(float x, float y, float w, float h);
    void move_to(float x, float y);
    void line_to(float x, float y);
    void stroke_path();

private:
    bool inited_ = false;
    bool null_mode_ = false;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    int w_ = 0;
    int h_ = 0;
    DrawList draws_;
};

} // namespace qianjs::platform
