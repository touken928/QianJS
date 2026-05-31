#include "platform/draw_list.h"

#include "nanovg.h"

#include <cmath>

namespace qianjs::platform {

namespace {

NVGcolor to_nvg(const Color4& c) {
    return nvgRGBA(static_cast<unsigned char>(c.r * 255.0f), static_cast<unsigned char>(c.g * 255.0f),
        static_cast<unsigned char>(c.b * 255.0f), static_cast<unsigned char>(c.a * 255.0f));
}

} // namespace

void DrawList::reset() {
    commands_.clear();
    path_scratch_.clear();
    path_storage_.clear();
    path_open_ = false;
    fill_style_ = {1, 1, 1, 1};
    stroke_style_ = {0, 0, 0, 1};
    line_width_ = 1.0f;
}

void DrawList::set_fill_style(const Color4& c) {
    fill_style_ = c;
}

void DrawList::set_stroke_style(const Color4& c) {
    stroke_style_ = c;
}

void DrawList::set_line_width(float w) {
    line_width_ = w;
}

void DrawList::clear_rect(float x, float y, float w, float h, const Color4& c) {
    DrawCommand cmd;
    cmd.op = DrawOp::ClearRect;
    cmd.a = x;
    cmd.b = y;
    cmd.c = w;
    cmd.d = h;
    cmd.fill = c;
    commands_.push_back(std::move(cmd));
}

void DrawList::fill_rect(float x, float y, float w, float h) {
    DrawCommand cmd;
    cmd.op = DrawOp::FillRect;
    cmd.a = x;
    cmd.b = y;
    cmd.c = w;
    cmd.d = h;
    cmd.fill = fill_style_;
    commands_.push_back(std::move(cmd));
}

void DrawList::stroke_rect(float x, float y, float w, float h) {
    DrawCommand cmd;
    cmd.op = DrawOp::StrokeRect;
    cmd.a = x;
    cmd.b = y;
    cmd.c = w;
    cmd.d = h;
    cmd.stroke = stroke_style_;
    cmd.line_width = line_width_;
    commands_.push_back(std::move(cmd));
}

void DrawList::begin_path() {
    path_scratch_.clear();
    path_open_ = true;
    DrawCommand cmd;
    cmd.op = DrawOp::BeginPath;
    commands_.push_back(std::move(cmd));
}

void DrawList::move_to(float x, float y) {
    if (!path_open_) {
        begin_path();
    }
    path_scratch_.emplace_back(x, y);
    DrawCommand cmd;
    cmd.op = DrawOp::MoveTo;
    cmd.a = x;
    cmd.b = y;
    commands_.push_back(std::move(cmd));
}

void DrawList::line_to(float x, float y) {
    path_scratch_.emplace_back(x, y);
    DrawCommand cmd;
    cmd.op = DrawOp::LineTo;
    cmd.a = x;
    cmd.b = y;
    commands_.push_back(std::move(cmd));
}

void DrawList::close_path() {
    DrawCommand cmd;
    cmd.op = DrawOp::ClosePath;
    commands_.push_back(std::move(cmd));
}

void DrawList::fill() {
    if (path_scratch_.size() < 2) {
        path_scratch_.clear();
        path_open_ = false;
        return;
    }
    const float off = static_cast<float>(path_storage_.size());
    const float count = static_cast<float>(path_scratch_.size());
    path_storage_.insert(path_storage_.end(), path_scratch_.begin(), path_scratch_.end());
    DrawCommand cmd;
    cmd.op = DrawOp::Fill;
    cmd.a = off;
    cmd.b = count;
    cmd.fill = fill_style_;
    commands_.push_back(std::move(cmd));
    path_scratch_.clear();
    path_open_ = false;
}

void DrawList::stroke() {
    if (path_scratch_.size() < 2) {
        path_scratch_.clear();
        path_open_ = false;
        return;
    }
    const float off = static_cast<float>(path_storage_.size());
    const float count = static_cast<float>(path_scratch_.size());
    path_storage_.insert(path_storage_.end(), path_scratch_.begin(), path_scratch_.end());
    DrawCommand cmd;
    cmd.op = DrawOp::Stroke;
    cmd.a = off;
    cmd.b = count;
    cmd.stroke = stroke_style_;
    cmd.line_width = line_width_;
    commands_.push_back(std::move(cmd));
    path_scratch_.clear();
    path_open_ = false;
}

void DrawList::save() {
    DrawCommand cmd;
    cmd.op = DrawOp::Save;
    commands_.push_back(std::move(cmd));
}

void DrawList::restore() {
    DrawCommand cmd;
    cmd.op = DrawOp::Restore;
    commands_.push_back(std::move(cmd));
}

void DrawList::translate(float tx, float ty) {
    DrawCommand cmd;
    cmd.op = DrawOp::Translate;
    cmd.a = tx;
    cmd.b = ty;
    commands_.push_back(std::move(cmd));
}

void DrawList::rotate(float rad) {
    DrawCommand cmd;
    cmd.op = DrawOp::Rotate;
    cmd.a = rad;
    commands_.push_back(std::move(cmd));
}

void DrawList::scale(float sx, float sy) {
    DrawCommand cmd;
    cmd.op = DrawOp::Scale;
    cmd.a = sx;
    cmd.b = sy;
    commands_.push_back(std::move(cmd));
}

void DrawList::fill_text(float x, float y, const std::string& text, float font_size) {
    DrawCommand cmd;
    cmd.op = DrawOp::FillText;
    cmd.a = x;
    cmd.b = y;
    cmd.fill = fill_style_;
    cmd.text = text;
    cmd.font_size = font_size;
    commands_.push_back(std::move(cmd));
}

void DrawList::execute(NVGcontext* vg, int font_face, float fb_width, float fb_height, float pixel_ratio) const {
    if (!vg) {
        return;
    }

    nvgBeginFrame(vg, fb_width, fb_height, pixel_ratio);

    auto replay_path = [&](size_t off, size_t count, bool closed) {
        if (off + count > path_storage_.size() || count < 1) {
            return;
        }
        nvgBeginPath(vg);
        nvgMoveTo(vg, path_storage_[off].first, path_storage_[off].second);
        for (size_t k = 1; k < count; ++k) {
            nvgLineTo(vg, path_storage_[off + k].first, path_storage_[off + k].second);
        }
        if (closed && count >= 2) {
            nvgClosePath(vg);
        }
    };

    for (const DrawCommand& cmd : commands_) {
        switch (cmd.op) {
        case DrawOp::ClearRect:
            nvgBeginPath(vg);
            nvgRect(vg, cmd.a, cmd.b, cmd.c, cmd.d);
            nvgFillColor(vg, to_nvg(cmd.fill));
            nvgFill(vg);
            break;
        case DrawOp::FillRect:
            nvgBeginPath(vg);
            nvgRect(vg, cmd.a, cmd.b, cmd.c, cmd.d);
            nvgFillColor(vg, to_nvg(cmd.fill));
            nvgFill(vg);
            break;
        case DrawOp::StrokeRect:
            nvgBeginPath(vg);
            nvgRect(vg, cmd.a, cmd.b, cmd.c, cmd.d);
            nvgStrokeColor(vg, to_nvg(cmd.stroke));
            nvgStrokeWidth(vg, cmd.line_width);
            nvgStroke(vg);
            break;
        case DrawOp::BeginPath:
            break;
        case DrawOp::MoveTo:
        case DrawOp::LineTo:
        case DrawOp::ClosePath:
            break;
        case DrawOp::Fill: {
            const size_t off = static_cast<size_t>(cmd.a);
            const size_t count = static_cast<size_t>(cmd.b);
            replay_path(off, count, true);
            nvgFillColor(vg, to_nvg(cmd.fill));
            nvgFill(vg);
            break;
        }
        case DrawOp::Stroke: {
            const size_t off = static_cast<size_t>(cmd.a);
            const size_t count = static_cast<size_t>(cmd.b);
            replay_path(off, count, false);
            nvgStrokeColor(vg, to_nvg(cmd.stroke));
            nvgStrokeWidth(vg, cmd.line_width);
            nvgStroke(vg);
            break;
        }
        case DrawOp::Save:
            nvgSave(vg);
            break;
        case DrawOp::Restore:
            nvgRestore(vg);
            break;
        case DrawOp::Translate:
            nvgTranslate(vg, cmd.a, cmd.b);
            break;
        case DrawOp::Rotate:
            nvgRotate(vg, cmd.a);
            break;
        case DrawOp::Scale:
            nvgScale(vg, cmd.a, cmd.b);
            break;
        case DrawOp::FillText:
            if (font_face >= 0 && !cmd.text.empty()) {
                nvgFontFaceId(vg, font_face);
                nvgFontSize(vg, cmd.font_size);
                nvgFillColor(vg, to_nvg(cmd.fill));
                nvgText(vg, cmd.a, cmd.b, cmd.text.c_str(), nullptr);
            }
            break;
        default:
            break;
        }
    }

    nvgEndFrame(vg);
}

} // namespace qianjs::platform
