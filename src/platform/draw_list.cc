#include "platform/draw_list.h"

#include <algorithm>
#include <cmath>

namespace qianjs::platform {

namespace {

Uint8 f2u8(float x) {
    const int v = static_cast<int>(std::lround(x * 255.0f));
    return static_cast<Uint8>(std::clamp(v, 0, 255));
}

void apply_color(SDL_Renderer* r, float cr, float cg, float cb, float ca) {
    SDL_SetRenderDrawColor(r, f2u8(cr), f2u8(cg), f2u8(cb), f2u8(ca));
}

void draw_line(SDL_Renderer* r, float line_width, float x0, float y0, float x1, float y1) {
    const int ix0 = static_cast<int>(std::lround(x0));
    const int iy0 = static_cast<int>(std::lround(y0));
    const int ix1 = static_cast<int>(std::lround(x1));
    const int iy1 = static_cast<int>(std::lround(y1));
    if (line_width <= 1.01f) {
        SDL_RenderDrawLine(r, ix0, iy0, ix1, iy1);
        return;
    }
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double len = std::hypot(dx, dy);
    if (len < 1e-9) {
        SDL_RenderDrawLine(r, ix0, iy0, ix1, iy1);
        return;
    }
    const double nx = (-dy / len) * 0.5 * line_width;
    const double ny = (dx / len) * 0.5 * line_width;
    const int steps = std::max(1, static_cast<int>(std::lround(line_width)));
    for (int s = 0; s < steps; ++s) {
        const double t = (static_cast<double>(s) + 0.5) / static_cast<double>(steps) - 0.5;
        const double ox = nx * t * 2.0;
        const double oy = ny * t * 2.0;
        SDL_RenderDrawLine(r, static_cast<int>(std::lround(x0 + ox)), static_cast<int>(std::lround(y0 + oy)),
            static_cast<int>(std::lround(x1 + ox)), static_cast<int>(std::lround(y1 + oy)));
    }
}

} // namespace

void DrawList::reset() {
    commands_.clear();
    path_scratch_.clear();
    path_storage_.clear();
    color_[0] = color_[1] = color_[2] = color_[3] = 1.0f;
    line_width_ = 1.0f;
}

void DrawList::clear_framebuffer(float r, float g, float b, float a) {
    commands_.push_back({DrawOp::Clear, r, g, b, a});
}

void DrawList::set_color(float r, float g, float b, float a) {
    color_[0] = r;
    color_[1] = g;
    color_[2] = b;
    color_[3] = a;
    commands_.push_back({DrawOp::SetColor, r, g, b, a});
}

void DrawList::set_line_width(float w) {
    line_width_ = std::max(1.0f, w);
    commands_.push_back({DrawOp::SetLineWidth, line_width_, 0, 0, 0});
}

void DrawList::fill_rect(float x, float y, float w, float h) {
    commands_.push_back({DrawOp::FillRect, x, y, w, h});
}

void DrawList::stroke_rect(float x, float y, float w, float h) {
    commands_.push_back({DrawOp::StrokeRect, x, y, w, h});
}

void DrawList::move_to(float x, float y) {
    path_scratch_.clear();
    path_scratch_.emplace_back(x, y);
}

void DrawList::line_to(float x, float y) {
    path_scratch_.emplace_back(x, y);
}

void DrawList::stroke_path() {
    if (path_scratch_.size() < 2) {
        path_scratch_.clear();
        return;
    }
    const float off = static_cast<float>(path_storage_.size());
    const float count = static_cast<float>(path_scratch_.size());
    path_storage_.insert(path_storage_.end(), path_scratch_.begin(), path_scratch_.end());
    commands_.push_back({DrawOp::StrokePath, off, count, 0, 0});
    path_scratch_.clear();
}

void DrawList::execute(SDL_Renderer* renderer) const {
    if (!renderer) {
        return;
    }

    float color[4] = {1, 1, 1, 1};
    float line_width = 1.0f;

    for (const DrawCommand& cmd : commands_) {
        switch (cmd.op) {
        case DrawOp::Clear:
            apply_color(renderer, cmd.a, cmd.b, cmd.c, cmd.d);
            SDL_RenderClear(renderer);
            apply_color(renderer, color[0], color[1], color[2], color[3]);
            break;
        case DrawOp::SetColor:
            color[0] = cmd.a;
            color[1] = cmd.b;
            color[2] = cmd.c;
            color[3] = cmd.d;
            apply_color(renderer, color[0], color[1], color[2], color[3]);
            break;
        case DrawOp::SetLineWidth:
            line_width = cmd.a;
            break;
        case DrawOp::FillRect: {
            apply_color(renderer, color[0], color[1], color[2], color[3]);
            SDL_Rect rc;
            rc.x = static_cast<int>(std::lround(cmd.a));
            rc.y = static_cast<int>(std::lround(cmd.b));
            rc.w = static_cast<int>(std::lround(cmd.c));
            rc.h = static_cast<int>(std::lround(cmd.d));
            SDL_RenderFillRect(renderer, &rc);
            break;
        }
        case DrawOp::StrokeRect: {
            apply_color(renderer, color[0], color[1], color[2], color[3]);
            SDL_Rect rc;
            rc.x = static_cast<int>(std::lround(cmd.a));
            rc.y = static_cast<int>(std::lround(cmd.b));
            rc.w = static_cast<int>(std::lround(cmd.c));
            rc.h = static_cast<int>(std::lround(cmd.d));
            if (line_width <= 1.01f) {
                SDL_RenderDrawRect(renderer, &rc);
            } else {
                const int n = std::max(1, static_cast<int>(std::lround(line_width)));
                for (int j = 0; j < n; ++j) {
                    SDL_Rect o = rc;
                    o.x -= j;
                    o.y -= j;
                    o.w += j * 2;
                    o.h += j * 2;
                    SDL_RenderDrawRect(renderer, &o);
                }
            }
            break;
        }
        case DrawOp::StrokePath: {
            const size_t off = static_cast<size_t>(cmd.a);
            const size_t count = static_cast<size_t>(cmd.b);
            apply_color(renderer, color[0], color[1], color[2], color[3]);
            if (off + count <= path_storage_.size() && count >= 2) {
                float px = path_storage_[off].first;
                float py = path_storage_[off].second;
                for (size_t k = 1; k < count; ++k) {
                    const float nx = path_storage_[off + k].first;
                    const float ny = path_storage_[off + k].second;
                    draw_line(renderer, line_width, px, py, nx, ny);
                    px = nx;
                    py = ny;
                }
            }
            break;
        }
        }
    }
}

} // namespace qianjs::platform
