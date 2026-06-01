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

void schedule_resolve_string_array(std::shared_ptr<qjs::Promise> ph, std::vector<std::string> names) {
    qianjs::event_loop::defer(
        [ph, names = std::move(names)](qjs::Engine& engine) {
            if (!ph) {
                return;
            }
            auto arr = engine.array();
            for (const auto& n : names) {
                arr.push(n);
            }
            ph->resolve(arr.build());
            release(ph.get());
        });
}

void schedule_resolve_stat(std::shared_ptr<qjs::Promise> ph, const uv_stat_t& st) {
    qianjs::event_loop::defer([ph, st](qjs::Engine& engine) {
        if (!ph) {
            return;
        }
        ph->resolve(fs_stat_to_value(engine, st));
        release(ph.get());
    });
}

} // namespace

qjs::Value fsReaddirAsync(qjs::Engine& engine, std::string path) {
    auto ph = qianjs::promise::create_shared(engine);
    if (!ph) {
        return {};
    }

    struct Ctx {
        std::shared_ptr<qjs::Promise> ph{};
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
        qianjs::event_loop::defer([ctx](qjs::Engine&) { ctx->req_keep.reset(); });
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
            qianjs::event_loop::defer([ctx](qjs::Engine&) { ctx->req_keep.reset(); });
            break;
        default:
            break;
        }
    });

    qianjs::event_loop::begin_operation();
    req->opendir(path);
    return ph->toValue();
}

qjs::Value fsStatAsync(qjs::Engine& engine, std::string path) {
    auto ph = qianjs::promise::create_shared(engine);
    if (!ph) {
        return {};
    }

    struct Ctx {
        std::shared_ptr<qjs::Promise> ph{};
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
        qianjs::event_loop::defer([ctx](qjs::Engine&) { ctx->req_keep.reset(); });
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::fs_req&) {
        if (ev.type == ft::STAT || ev.type == ft::LSTAT) {
            qianjs::event_loop::end_operation();
            schedule_resolve_stat(ctx->ph, ev.stat);
            qianjs::event_loop::defer([ctx](qjs::Engine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    req->stat(path);
    return ph->toValue();
}

static qjs::Value fs_one_path_void(qjs::Engine& engine, std::string path,
    void (*start)(uvw::fs_req&, const std::string&), uvw::fs_req::fs_type doneType) {
    auto ph = qianjs::promise::create_shared(engine);
    if (!ph) {
        return {};
    }

    struct Ctx {
        std::shared_ptr<qjs::Promise> ph{};
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
        qianjs::event_loop::defer([ctx](qjs::Engine&) { ctx->req_keep.reset(); });
    });

    req->on<uvw::fs_event>([ctx, doneType](const uvw::fs_event& ev, uvw::fs_req&) {
        if (ev.type == doneType) {
            qianjs::event_loop::end_operation();
            resolve_void(ctx->ph);
            qianjs::event_loop::defer([ctx](qjs::Engine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    start(*req, path);
    return ph->toValue();
}

qjs::Value fsUnlinkAsync(qjs::Engine& engine, std::string path) {
    return fs_one_path_void(
        engine, std::move(path), [](uvw::fs_req& r, const std::string& p) { r.unlink(p); }, uvw::fs_req::fs_type::UNLINK);
}

qjs::Value fsRmdirAsync(qjs::Engine& engine, std::string path) {
    return fs_one_path_void(
        engine, std::move(path), [](uvw::fs_req& r, const std::string& p) { r.rmdir(p); }, uvw::fs_req::fs_type::RMDIR);
}
