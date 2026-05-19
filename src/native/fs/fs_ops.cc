#include "native/fs/fs_stat_js.h"
#include "native/fs/fs_uv.h"

#include "native/fs/fs_async_schedule.h"

#include "runtime/event_loop/event_loop.h"
#include "runtime/promise_track.h"

#include <uvw.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

using qianjs::fs::schedule::reject;
using qianjs::fs::schedule::resolve_void;
using qianjs::promise::release;

void schedule_resolve_string_array(qjs::JSEngine::PromiseHandle ph, std::vector<std::string> names) {
    qianjs::event_loop::defer(
        [ph, names = std::move(names)](qjs::JSEngine& e) {
            JSContext* c = e.ctx();
            JSValue arr = JS_NewArray(c);
            if (JS_IsException(arr)) {
                e.rejectPromise(ph, "failed to allocate array");
                release(e, ph);
                return;
            }
            for (uint32_t i = 0; i < names.size(); i++) {
                JSValue s = JS_NewString(c, names[i].c_str());
                if (JS_IsException(s) || JS_SetPropertyUint32(c, arr, i, s) < 0) {
                    JS_FreeValue(c, arr);
                    e.rejectPromise(ph, "failed to build readdir result");
                    release(e, ph);
                    return;
                }
            }
            e.resolvePromiseJSValue(ph, arr);
            release(e, ph);
        });
}

void schedule_resolve_stat(qjs::JSEngine::PromiseHandle ph, const uv_stat_t& st) {
    qianjs::event_loop::defer([ph, st](qjs::JSEngine& e) {
        JSContext* c = e.ctx();
        JSValue o = fs_stat_to_js(c, st);
        if (JS_IsException(o)) {
            e.rejectPromise(ph, "failed to build stat object");
            release(e, ph);
            return;
        }
        e.resolvePromiseJSValue(ph, o);
        release(e, ph);
    });
}

} // namespace

qjs::RawJSValue fsReaddirAsync(qjs::JSEngine& engine, std::string path) {
    qjs::JSEngine::PromiseHandle ph = qianjs::promise::create(engine);
    if (!ph.ptr)
        return engine.promiseValue(ph);

    struct Ctx {
        qjs::JSEngine::PromiseHandle ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
        std::vector<std::string> names;
    };
    auto ctx = std::make_shared<Ctx>();
    ctx->ph = ph;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    ctx->req_keep = loop->resource<uvw::fs_req>();

    using ft = uvw::fs_req::fs_type;
    auto req = ctx->req_keep;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        qianjs::event_loop::end_operation();
        reject(ctx->ph, e.what());
        qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::fs_req& r) {
        switch (ev.type) {
        case ft::OPENDIR:
            r.readdir();
            break;
        case ft::READDIR:
            if (ev.dirent.eos) {
                r.closedir();
            } else if (ev.dirent.name) {
                ctx->names.emplace_back(ev.dirent.name);
                r.readdir();
            } else {
                r.closedir();
            }
            break;
        case ft::CLOSEDIR:
            qianjs::event_loop::end_operation();
            schedule_resolve_string_array(ctx->ph, std::move(ctx->names));
            qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
            break;
        default:
            break;
        }
    });

    qianjs::event_loop::begin_operation();
    req->opendir(path);
    return engine.promiseValue(ph);
}

qjs::RawJSValue fsStatAsync(qjs::JSEngine& engine, std::string path) {
    qjs::JSEngine::PromiseHandle ph = qianjs::promise::create(engine);
    if (!ph.ptr)
        return engine.promiseValue(ph);

    struct Ctx {
        qjs::JSEngine::PromiseHandle ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
    };
    auto ctx = std::make_shared<Ctx>();
    ctx->ph = ph;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::fs_req>();
    ctx->req_keep = req;

    using ft = uvw::fs_req::fs_type;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        qianjs::event_loop::end_operation();
        reject(ctx->ph, e.what());
        qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::fs_req&) {
        if (ev.type == ft::STAT || ev.type == ft::LSTAT) {
            qianjs::event_loop::end_operation();
            schedule_resolve_stat(ctx->ph, ev.stat);
            qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    req->stat(path);
    return engine.promiseValue(ph);
}

static qjs::RawJSValue fs_one_path_void(qjs::JSEngine& engine, std::string path,
    void (*start)(uvw::fs_req&, const std::string&), uvw::fs_req::fs_type doneType) {
    qjs::JSEngine::PromiseHandle ph = qianjs::promise::create(engine);
    if (!ph.ptr)
        return engine.promiseValue(ph);

    struct Ctx {
        qjs::JSEngine::PromiseHandle ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
    };
    auto ctx = std::make_shared<Ctx>();
    ctx->ph = ph;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::fs_req>();
    ctx->req_keep = req;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        qianjs::event_loop::end_operation();
        reject(ctx->ph, e.what());
        qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
    });

    req->on<uvw::fs_event>([ctx, doneType](const uvw::fs_event& ev, uvw::fs_req&) {
        if (ev.type == doneType) {
            qianjs::event_loop::end_operation();
            resolve_void(ctx->ph);
            qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    start(*req, path);
    return engine.promiseValue(ph);
}

qjs::RawJSValue fsUnlinkAsync(qjs::JSEngine& engine, std::string path) {
    return fs_one_path_void(
        engine, std::move(path), [](uvw::fs_req& r, const std::string& p) { r.unlink(p); }, uvw::fs_req::fs_type::UNLINK);
}

qjs::RawJSValue fsRmdirAsync(qjs::JSEngine& engine, std::string path) {
    return fs_one_path_void(
        engine, std::move(path), [](uvw::fs_req& r, const std::string& p) { r.rmdir(p); }, uvw::fs_req::fs_type::RMDIR);
}

