#include "native/canvas/canvas_module.h"

#include "native/canvas/canvas_color.h"
#include "platform/canvas_access.h"
#include "platform/js_bridge.h"
#include "platform/platform_canvas.h"
#include "runtime/instance.h"

#include <qjs/engine.h>
#include <qjs/module.h>
#include <qjs/object.h>
#include <qjs/value.h>

#include <functional>
#include <optional>
#include <string>

#include <SDL.h>

namespace {

using qianjs::canvas::canvas_by_id;
using qianjs::canvas::parse_color;

qjs::Value build_2d_context(qjs::Engine& engine, uint64_t canvas_id) {
    qjs::ObjectBuilder b(engine);
    b.set("_canvasId", static_cast<int64_t>(canvas_id));

    b.func("clearRect", [canvas_id](double x, double y, double w, double h) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().clear_rect(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                static_cast<float>(h), c->fill_style());
        }
    });
    b.func("fillRect", [canvas_id](double x, double y, double w, double h) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().fill_rect(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                static_cast<float>(h));
        }
    });
    b.func("strokeRect", [canvas_id](double x, double y, double w, double h) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().stroke_rect(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                static_cast<float>(h));
        }
    });
    b.func("beginPath", std::function<void()>([canvas_id]() {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().begin_path();
        }
    }));
    b.func("moveTo", [canvas_id](double x, double y) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().move_to(static_cast<float>(x), static_cast<float>(y));
        }
    });
    b.func("lineTo", [canvas_id](double x, double y) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().line_to(static_cast<float>(x), static_cast<float>(y));
        }
    });
    b.func("closePath", std::function<void()>([canvas_id]() {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().close_path();
        }
    }));
    b.func("fill", std::function<void()>([canvas_id]() {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().fill();
        }
    }));
    b.func("stroke", std::function<void()>([canvas_id]() {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().stroke();
        }
    }));
    b.func("save", std::function<void()>([canvas_id]() {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().save();
        }
    }));
    b.func("restore", std::function<void()>([canvas_id]() {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().restore();
        }
    }));
    b.func("translate", [canvas_id](double x, double y) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().translate(static_cast<float>(x), static_cast<float>(y));
        }
    });
    b.func("rotate", [canvas_id](double a) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().rotate(static_cast<float>(a));
        }
    });
    b.func("scale", [canvas_id](double sx, double sy) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().scale(static_cast<float>(sx), static_cast<float>(sy));
        }
    });
    b.func("fillText", [canvas_id](std::string text, double x, double y) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->draw_list().fill_text(static_cast<float>(x), static_cast<float>(y), text, c->font_size());
        }
    });
    b.func("measureText", std::function<qjs::Value(std::string)>([&engine, canvas_id](std::string text) -> qjs::Value {
        float font = 16.f;
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            font = c->font_size();
        }
        const float est = static_cast<float>(text.size()) * font * 0.55f;
        return engine.object().set("width", est).build();
    }));
    b.func("setFillStyle", std::function<void(qjs::Value)>([&engine, canvas_id](qjs::Value arg) {
        qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id);
        if (!c) {
            return;
        }
        auto col = parse_color(arg);
        if (!col.success) {
            (void)engine.throwTypeError(col.error.message);
            return;
        }
        auto s = arg.toString();
        std::string css = s.success ? std::move(s.value) : std::string{};
        c->set_fill_style(col.value, css);
    }));
    b.func("setStrokeStyle", std::function<void(qjs::Value)>([&engine, canvas_id](qjs::Value arg) {
        qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id);
        if (!c) {
            return;
        }
        auto col = parse_color(arg);
        if (!col.success) {
            (void)engine.throwTypeError(col.error.message);
            return;
        }
        auto s = arg.toString();
        std::string css = s.success ? std::move(s.value) : std::string{};
        c->set_stroke_style(col.value, css);
    }));
    b.func("getFillStyle", std::function<qjs::Value()>([&engine, canvas_id]() -> qjs::Value {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            return engine.string(c->fill_style_css());
        }
        return engine.undefined();
    }));
    b.func("getStrokeStyle", std::function<qjs::Value()>([&engine, canvas_id]() -> qjs::Value {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            return engine.string(c->stroke_style_css());
        }
        return engine.undefined();
    }));
    b.func("setLineWidth", [canvas_id](double w) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            c->set_line_width(static_cast<float>(w));
        }
    });
    b.func("getLineWidth", std::function<qjs::Value()>([&engine, canvas_id]() -> qjs::Value {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            return engine.float64(c->line_width());
        }
        return engine.undefined();
    }));
    b.func("setFont", [canvas_id](std::string s) {
        if (qianjs::platform::PlatformCanvas* c = canvas_by_id(canvas_id)) {
            const size_t px = s.find("px");
            if (px != std::string::npos) {
                try {
                    c->set_font_size(std::stof(s.substr(0, px)));
                } catch (...) {
                }
            }
        }
    });

    return b.build();
}

std::string title_from_options(const qjs::Value& opt) {
    if (opt.isObject()) {
        auto ts = opt.getProperty("title");
        if (ts.success && !ts.value.isUndefined()) {
            auto s = ts.value.toString();
            if (s.success) {
                return std::move(s.value);
            }
        }
    } else {
        auto s = opt.toString();
        if (s.success) {
            return std::move(s.value);
        }
    }
    return "QianJS";
}

} // namespace

const char* CanvasPlugin::name() const {
    return "canvas";
}

void CanvasPlugin::install(qjs::Context& ctx, qjs::Module& root) {
    qjs::Engine& eng = ctx.engine();
    auto& m = root.module("canvas");

    m.func("createCanvas",
        std::function<qjs::Value(int, int, std::optional<qjs::Value>)>([&eng](int w, int h,
                                                                              std::optional<qjs::Value> options) -> qjs::Value {
        if (w <= 0 || h <= 0) {
            return eng.throwTypeError("createCanvas: width and height must be positive");
        }
        std::string title = "QianJS";
        if (options) {
            title = title_from_options(*options);
        }

        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return eng.throwTypeError("createCanvas: no active runtime instance");
        }
        const uint64_t id = inst->canvases().create(w, h, title);
        if (id == 0) {
            return eng.throwTypeError("createCanvas: failed to create window");
        }

        return eng.object()
            .set("width", w)
            .set("height", h)
            .set("_canvasId", static_cast<int64_t>(id))
            .func("getContext", std::function<qjs::Value(std::string)>([&eng, id](std::string type) -> qjs::Value {
                if (type != "2d") {
                    return eng.undefined();
                }
                return build_2d_context(eng, id);
            }))
            .func("present", std::function<void()>([id]() {
                if (qianjs::platform::PlatformCanvas* c = canvas_by_id(id)) {
                    c->present();
                }
            }))
            .func("pollEvents", std::function<qjs::Value()>([&eng, id]() -> qjs::Value {
                qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
                if (!inst) {
                    return eng.throwTypeError("pollEvents: no active runtime instance");
                }
                qianjs::platform::PlatformCanvas* canvas = canvas_by_id(id);
                if (!canvas || !canvas->inited()) {
                    return eng.throwTypeError("pollEvents: invalid canvas");
                }
                std::vector<SDL_Event> batch = inst->canvases().take_events(id);
                const bool should_close = qianjs::platform::PlatformCanvas::batch_quit_or_escape(
                    batch, canvas->sdl_window_id());
                return eng.object()
                    .set("events",
                        qianjs::platform::PlatformCanvas::events_to_js(eng, batch, canvas->sdl_window_id()))
                    .set("shouldClose", should_close)
                    .build();
            }))
            .func("close", std::function<void()>([id]() {
                if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
                    inst->canvases().destroy(id);
                }
            }))
            .build();
    }));

    m.func("pumpEvents", std::function<qjs::Value()>([&eng]() -> qjs::Value {
        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return eng.throwTypeError("pumpEvents: no active runtime instance");
        }
        inst->canvases().pump_events();
        return eng.object().set("quit", inst->canvases().quit_requested()).build();
    }));
}
