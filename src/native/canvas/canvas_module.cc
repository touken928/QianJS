#include "native/canvas/canvas_module.h"

#include "native/canvas/canvas_color.h"
#include "platform/canvas_access.h"
#include "platform/js_bridge.h"
#include "platform/platform_canvas.h"
#include "runtime/instance.h"

#include <qjs/call.h>
#include <qjs/engine.h>
#include <qjs/module.h>
#include <qjs/object.h>

#include <cmath>
#include <string>

namespace {

using qianjs::canvas::parse_color;
using qianjs::canvas::require_canvas;
using qianjs::canvas::require_canvas_id;

template <typename Fn>
auto bind_canvas(uint64_t canvas_id, Fn fn) {
    return [canvas_id, fn = std::move(fn)](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto* c = require_canvas_id(ctx, canvas_id);
        if (!c) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"invalid canvas", {}, {}});
        }
        return fn(ctx, c);
    };
}

qjs::Value build_2d_context(qjs::Engine& engine, uint64_t canvas_id) {
    qjs::ObjectBuilder b(engine);
    b.setInt64("_canvasId", static_cast<int64_t>(canvas_id));

    b.funcDynamic("clearRect", 4, 4,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto x = ctx.float64Arg(0);
            auto y = ctx.float64Arg(1);
            auto w = ctx.float64Arg(2);
            auto h = ctx.float64Arg(3);
            if (!x.success || !y.success || !w.success || !h.success) {
                return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"clearRect: expected 4 numbers", {}, {}});
            }
            c->draw_list().clear_rect(static_cast<float>(x.value), static_cast<float>(y.value),
                static_cast<float>(w.value), static_cast<float>(h.value), c->fill_style());
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("fillRect", 4, 4,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto x = ctx.float64Arg(0);
            auto y = ctx.float64Arg(1);
            auto w = ctx.float64Arg(2);
            auto h = ctx.float64Arg(3);
            if (!x.success || !y.success || !w.success || !h.success) {
                return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"fillRect: expected 4 numbers", {}, {}});
            }
            c->draw_list().fill_rect(static_cast<float>(x.value), static_cast<float>(y.value),
                static_cast<float>(w.value), static_cast<float>(h.value));
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("strokeRect", 4, 4,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto x = ctx.float64Arg(0);
            auto y = ctx.float64Arg(1);
            auto w = ctx.float64Arg(2);
            auto h = ctx.float64Arg(3);
            if (!x.success || !y.success || !w.success || !h.success) {
                return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"strokeRect: expected 4 numbers", {}, {}});
            }
            c->draw_list().stroke_rect(static_cast<float>(x.value), static_cast<float>(y.value),
                static_cast<float>(w.value), static_cast<float>(h.value));
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("beginPath", 0, 0, bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
        c->draw_list().begin_path();
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));

    b.funcDynamic("moveTo", 2, 2,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto x = ctx.float64Arg(0);
            auto y = ctx.float64Arg(1);
            if (!x.success || !y.success) {
                return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"moveTo: expected 2 numbers", {}, {}});
            }
            c->draw_list().move_to(static_cast<float>(x.value), static_cast<float>(y.value));
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("lineTo", 2, 2,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto x = ctx.float64Arg(0);
            auto y = ctx.float64Arg(1);
            if (!x.success || !y.success) {
                return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"lineTo: expected 2 numbers", {}, {}});
            }
            c->draw_list().line_to(static_cast<float>(x.value), static_cast<float>(y.value));
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("closePath", 0, 0, bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
        c->draw_list().close_path();
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));

    b.funcDynamic("fill", 0, 0, bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
        c->draw_list().fill();
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));

    b.funcDynamic("stroke", 0, 0, bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
        c->draw_list().stroke();
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));

    b.funcDynamic("save", 0, 0, bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
        c->draw_list().save();
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));

    b.funcDynamic("restore", 0, 0, bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
        c->draw_list().restore();
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));

    b.funcDynamic("translate", 2, 2,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto x = ctx.float64Arg(0);
            auto y = ctx.float64Arg(1);
            if (!x.success || !y.success) {
                return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"translate: expected 2 numbers", {}, {}});
            }
            c->draw_list().translate(static_cast<float>(x.value), static_cast<float>(y.value));
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("rotate", 1, 1,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto a = ctx.float64Arg(0);
            if (!a.success) {
                return qjs::Result<qjs::Value>::fail(a.error);
            }
            c->draw_list().rotate(static_cast<float>(a.value));
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("scale", 2, 2,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto sx = ctx.float64Arg(0);
            auto sy = ctx.float64Arg(1);
            if (!sx.success || !sy.success) {
                return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"scale: expected 2 numbers", {}, {}});
            }
            c->draw_list().scale(static_cast<float>(sx.value), static_cast<float>(sy.value));
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("fillText", 3, 3,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto text = ctx.stringArg(0);
            auto x = ctx.float64Arg(1);
            auto y = ctx.float64Arg(2);
            if (!text.success || !x.success || !y.success) {
                return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"fillText: expected (text, x, y)", {}, {}});
            }
            c->draw_list().fill_text(static_cast<float>(x.value), static_cast<float>(y.value), text.value, c->font_size());
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("measureText", 1, 1,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto text = ctx.stringArg(0);
            if (!text.success) {
                return qjs::Result<qjs::Value>::fail(text.error);
            }
            const float est = static_cast<float>(text.value.size()) * c->font_size() * 0.55f;
            qjs::ObjectBuilder out(ctx.engine());
            out.setDouble("width", est);
            return qjs::Result<qjs::Value>::ok(out.build());
        }));

    b.funcDynamic("setFillStyle", 1, 1,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto arg = ctx.valueArg(0);
            if (!arg.success) {
                return qjs::Result<qjs::Value>::fail(arg.error);
            }
            auto col = parse_color(arg.value);
            if (!col.success) {
                return qjs::Result<qjs::Value>::fail(col.error);
            }
            auto s = arg.value.toString();
            std::string css = s.success ? std::move(s.value) : std::string{};
            c->set_fill_style(col.value, css);
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("setStrokeStyle", 1, 1,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto arg = ctx.valueArg(0);
            if (!arg.success) {
                return qjs::Result<qjs::Value>::fail(arg.error);
            }
            auto col = parse_color(arg.value);
            if (!col.success) {
                return qjs::Result<qjs::Value>::fail(col.error);
            }
            auto s = arg.value.toString();
            std::string css = s.success ? std::move(s.value) : std::string{};
            c->set_stroke_style(col.value, css);
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("getFillStyle", 0, 0, bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
        return qjs::Result<qjs::Value>::ok(ctx.engine().string(c->fill_style_css()));
    }));

    b.funcDynamic("getStrokeStyle", 0, 0, bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
        return qjs::Result<qjs::Value>::ok(ctx.engine().string(c->stroke_style_css()));
    }));

    b.funcDynamic("setLineWidth", 1, 1,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto w = ctx.float64Arg(0);
            if (!w.success) {
                return qjs::Result<qjs::Value>::fail(w.error);
            }
            c->set_line_width(static_cast<float>(w.value));
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    b.funcDynamic("getLineWidth", 0, 0, bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
        return qjs::Result<qjs::Value>::ok(ctx.engine().float64(c->line_width()));
    }));

    b.funcDynamic("setFont", 1, 1,
        bind_canvas(canvas_id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) -> qjs::Result<qjs::Value> {
            auto s = ctx.stringArg(0);
            if (!s.success) {
                return qjs::Result<qjs::Value>::fail(s.error);
            }
            const size_t px = s.value.find("px");
            if (px != std::string::npos) {
                try {
                    c->set_font_size(std::stof(s.value.substr(0, px)));
                } catch (...) {
                }
            }
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }));

    return b.build();
}

} // namespace

const char* CanvasPlugin::name() const {
    return "canvas";
}

void CanvasPlugin::install(qjs::Context&, qjs::Module& root) {
    auto& m = root.module("canvas");

    m.funcDynamic("createCanvas", 2, 3, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto w = ctx.int32Arg(0);
        auto h = ctx.int32Arg(1);
        if (!w.success || !h.success) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"createCanvas: width and height required", {}, {}});
        }
        if (w.value <= 0 || h.value <= 0) {
            return qjs::Result<qjs::Value>::fail(
                qjs::ErrorInfo{"createCanvas: width and height must be positive", {}, {}});
        }
        std::string title = "QianJS";
        if (ctx.argc() >= 3) {
            auto t = ctx.valueArg(2);
            if (t.success && t.value.isObject()) {
                auto ts = t.value.getProperty("title");
                if (ts.success && !ts.value.isUndefined()) {
                    auto s = ts.value.toString();
                    if (s.success) {
                        title = std::move(s.value);
                    }
                }
            } else if (t.success) {
                auto s = t.value.toString();
                if (s.success) {
                    title = std::move(s.value);
                }
            }
        }

        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return qjs::Result<qjs::Value>::fail(
                qjs::ErrorInfo{"createCanvas: no active runtime instance", {}, {}});
        }
        const uint64_t id = inst->canvases().create(w.value, h.value, title);
        if (id == 0) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"createCanvas: failed to create window", {}, {}});
        }

        qjs::Engine& engine = ctx.engine();
        auto canvas_obj = engine.object()
                              .setInt64("width", w.value)
                              .setInt64("height", h.value)
                              .setInt64("_canvasId", static_cast<int64_t>(id))
                              .funcDynamic("getContext", 1, 1,
                                  [id](qjs::CallContext& c) -> qjs::Result<qjs::Value> {
                                      auto type = c.stringArg(0);
                                      if (!type.success) {
                                          return qjs::Result<qjs::Value>::fail(type.error);
                                      }
                                      if (type.value != "2d") {
                                          return qjs::Result<qjs::Value>::ok(c.undefined());
                                      }
                                      return qjs::Result<qjs::Value>::ok(build_2d_context(c.engine(), id));
                                  })
                              .funcDynamic("present", 0, 0,
                                  bind_canvas(id, [](qjs::CallContext& ctx, qianjs::platform::PlatformCanvas* c) {
                                      c->present();
                                      return qjs::Result<qjs::Value>::ok(ctx.undefined());
                                  }))
                              .funcDynamic("pollEvents", 0, 0,
                                  [id](qjs::CallContext& c) -> qjs::Result<qjs::Value> {
                                      qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
                                      if (!inst) {
                                          return qjs::Result<qjs::Value>::fail(
                                              qjs::ErrorInfo{"pollEvents: no active runtime instance", {}, {}});
                                      }
                                      qianjs::platform::PlatformCanvas* canvas = qianjs::canvas::canvas_by_id(id);
                                      if (!canvas || !canvas->inited()) {
                                          return qjs::Result<qjs::Value>::fail(
                                              qjs::ErrorInfo{"pollEvents: invalid canvas", {}, {}});
                                      }
                                      std::vector<SDL_Event> batch = inst->canvases().take_events(id);
                                      const bool should_close = qianjs::platform::PlatformCanvas::batch_quit_or_escape(
                                          batch, canvas->sdl_window_id());
                                      qjs::ObjectBuilder out(c.engine());
                                      out.set("events", qianjs::platform::PlatformCanvas::events_to_js(
                                                              c.engine(), batch, canvas->sdl_window_id()));
                                      out.setBool("shouldClose", should_close);
                                      return qjs::Result<qjs::Value>::ok(out.build());
                                  })
                              .funcDynamic("close", 0, 0,
                                  [id](qjs::CallContext& c) -> qjs::Result<qjs::Value> {
                                      if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
                                          inst->canvases().destroy(id);
                                      }
                                      return qjs::Result<qjs::Value>::ok(c.undefined());
                                  })
                              .build();
        return qjs::Result<qjs::Value>::ok(std::move(canvas_obj));
    });

    m.funcDynamic("pumpEvents", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{"pumpEvents: no active runtime instance", {}, {}});
        }
        inst->canvases().pump_events();
        return qjs::Result<qjs::Value>::ok(ctx.engine().object().setBool("quit", inst->canvases().quit_requested()).build());
    });
}
