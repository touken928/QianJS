#pragma once

struct NVGcontext;

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace qianjs::platform {

enum class DrawOp : uint8_t {
    ClearRect,
    SetFillStyle,
    SetStrokeStyle,
    SetLineWidth,
    FillRect,
    StrokeRect,
    BeginPath,
    MoveTo,
    LineTo,
    ClosePath,
    Fill,
    Stroke,
    Save,
    Restore,
    Translate,
    Rotate,
    Scale,
    FillText,
};

struct Color4 {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;
};

/** Retained Canvas 2D draw commands; replayed with NanoVG on present. */
class DrawList {
public:
    void reset();

    void clear_rect(float x, float y, float w, float h, const Color4& c);
    void set_fill_style(const Color4& c);
    void set_stroke_style(const Color4& c);
    void set_line_width(float w);

    void fill_rect(float x, float y, float w, float h);
    void stroke_rect(float x, float y, float w, float h);

    void begin_path();
    void move_to(float x, float y);
    void line_to(float x, float y);
    void close_path();
    void fill();
    void stroke();

    void save();
    void restore();
    void translate(float tx, float ty);
    void rotate(float rad);
    void scale(float sx, float sy);

    void fill_text(float x, float y, const std::string& text, float font_size);

    void execute(NVGcontext* vg, int font_face, float fb_width, float fb_height, float pixel_ratio) const;

private:
    struct DrawCommand {
        DrawOp op = DrawOp::ClearRect;
        float a = 0;
        float b = 0;
        float c = 0;
        float d = 0;
        Color4 fill{};
        Color4 stroke{};
        float line_width = 1.0f;
        std::string text;
        float font_size = 16.0f;
    };

    Color4 fill_style_{1, 1, 1, 1};
    Color4 stroke_style_{0, 0, 0, 1};
    float line_width_ = 1.0f;

    std::vector<DrawCommand> commands_;
    std::vector<std::pair<float, float>> path_scratch_;
    std::vector<std::pair<float, float>> path_storage_;
    bool path_open_ = false;
};

} // namespace qianjs::platform
