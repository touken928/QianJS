#pragma once

#include <SDL.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace qianjs::platform {

enum class DrawOp : uint8_t {
    Clear,
    SetColor,
    SetLineWidth,
    FillRect,
    StrokeRect,
    StrokePath,
};

struct DrawCommand {
    DrawOp op = DrawOp::Clear;
    float a = 0;
    float b = 0;
    float c = 0;
    float d = 0;
};

/** Retained 2D draw commands; executed on the main thread during present. */
class DrawList {
public:
    void reset();

    void clear_framebuffer(float r, float g, float b, float a);
    void set_color(float r, float g, float b, float a);
    void set_line_width(float w);
    void fill_rect(float x, float y, float w, float h);
    void stroke_rect(float x, float y, float w, float h);
    void move_to(float x, float y);
    void line_to(float x, float y);
    void stroke_path();

    void execute(SDL_Renderer* renderer) const;

private:
    std::vector<DrawCommand> commands_;
    std::vector<std::pair<float, float>> path_scratch_;
    std::vector<std::pair<float, float>> path_storage_;
    float color_[4] = {1, 1, 1, 1};
    float line_width_ = 1.0f;
};

} // namespace qianjs::platform
