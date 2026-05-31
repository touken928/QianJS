#include "native/fs/fs_uv.h"

#include "native/fs/fs_async_schedule.h"

#include "runtime/event_loop/event_loop.h"
#include "runtime/promise_track.h"

#include <uvw.hpp>

#include <climits>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <sys/stat.h>

namespace {

using file_flags = uvw::file_req::file_open_flags;
using qianjs::fs::schedule::reject;
using qianjs::fs::schedule::resolve_bytes;
using qianjs::fs::schedule::resolve_string;
using qianjs::fs::schedule::resolve_void;

static bool stat_is_dir(const uv_stat_t& st) {
#ifdef S_ISDIR
    return S_ISDIR(static_cast<unsigned>(st.st_mode));
#else
    return (st.st_mode & _S_IFMT) == _S_IFDIR;
#endif
}

struct FsReadCtx {
    std::shared_ptr<qjs::Promise> ph{};
    std::string buffer;
    std::string fail_on_close;
    std::shared_ptr<uvw::file_req> req_keep;
    bool as_buffer = false;
};

void defer_release_read_req(const std::shared_ptr<FsReadCtx>& ctx) {
    qianjs::event_loop::defer([ctx](qjs::Engine&) { ctx->req_keep.reset(); });
}

void read_done_fail(const std::shared_ptr<FsReadCtx>& ctx, std::string msg) {
    qianjs::event_loop::end_operation();
    reject(ctx->ph, std::move(msg));
    defer_release_read_req(ctx);
}

void read_resolve_ok(const std::shared_ptr<FsReadCtx>& ctx) {
    qianjs::event_loop::end_operation();
    if (ctx->as_buffer)
        resolve_bytes(ctx->ph, std::move(ctx->buffer));
    else
        resolve_string(ctx->ph, std::move(ctx->buffer));
    defer_release_read_req(ctx);
}

struct FsWriteCtx {
    std::shared_ptr<qjs::Promise> ph{};
    std::vector<uint8_t> data;
    std::shared_ptr<uvw::file_req> req_keep;
};

void defer_release_write_req(const std::shared_ptr<FsWriteCtx>& ctx) {
    qianjs::event_loop::defer([ctx](qjs::Engine&) { ctx->req_keep.reset(); });
}

void write_done_fail(const std::shared_ptr<FsWriteCtx>& ctx, std::string msg) {
    qianjs::event_loop::end_operation();
    reject(ctx->ph, std::move(msg));
    defer_release_write_req(ctx);
}

} // namespace

qjs::Value fsReadFileAsync(qjs::Engine& engine, std::string path, bool asBuffer) {
    auto ph = qianjs::promise::create_shared(engine);
    if (!ph) {
        return {};
    }

    auto ctx = std::make_shared<FsReadCtx>();
    ctx->ph = ph;
    ctx->as_buffer = asBuffer;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::file_req>();
    ctx->req_keep = req;

    using ft = uvw::file_req::fs_type;
    const file_flags openFlags = file_flags::RDONLY;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        read_done_fail(ctx, e.what());
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::file_req& r) {
        switch (ev.type) {
        case ft::OPEN:
            r.stat();
            break;
        case ft::FSTAT: {
            const uint64_t sz64 = static_cast<uint64_t>(ev.stat.st_size);
            if (stat_is_dir(ev.stat)) {
                ctx->fail_on_close = "EISDIR: illegal operation on a directory";
                r.close();
                break;
            }
            if (sz64 == 0) {
                ctx->buffer.clear();
                r.close();
                break;
            }
            if (sz64 > static_cast<uint64_t>(SIZE_MAX)) {
                read_done_fail(ctx, "file too large");
                break;
            }
            const std::size_t sz = static_cast<std::size_t>(sz64);
            if (sz > static_cast<std::size_t>(UINT_MAX)) {
                read_done_fail(ctx, "file too large for single read");
                break;
            }
            r.read(0, static_cast<unsigned>(sz));
            break;
        }
        case ft::READ:
            if (ev.result > 0 && ev.read.data)
                ctx->buffer.assign(ev.read.data.get(), ev.read.data.get() + ev.result);
            else
                ctx->buffer.clear();
            r.close();
            break;
        case ft::CLOSE:
            if (!ctx->fail_on_close.empty()) {
                auto m = std::move(ctx->fail_on_close);
                read_done_fail(ctx, std::move(m));
            } else {
                read_resolve_ok(ctx);
            }
            break;
        default:
            break;
        }
    });

    qianjs::event_loop::begin_operation();
    req->open(path, openFlags, 0);
    return ph->toValue();
}

static qjs::Value write_file_impl(qjs::Engine& engine, std::string path, std::vector<uint8_t> data, file_flags flags,
    int mode) {
    auto ph = qianjs::promise::create_shared(engine);
    if (!ph) {
        return {};
    }

    auto ctx = std::make_shared<FsWriteCtx>();
    ctx->ph = ph;
    ctx->data = std::move(data);
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::file_req>();
    ctx->req_keep = req;

    using ft = uvw::file_req::fs_type;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        write_done_fail(ctx, e.what());
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::file_req& r) {
        switch (ev.type) {
        case ft::OPEN: {
            const unsigned len = static_cast<unsigned>(ctx->data.size());
            if (len == 0) {
                auto buf = std::make_unique<char[]>(1);
                r.write(std::move(buf), 0, 0);
            } else {
                auto buf = std::make_unique<char[]>(ctx->data.size());
                std::memcpy(buf.get(), ctx->data.data(), ctx->data.size());
                r.write(std::move(buf), len, 0);
            }
            break;
        }
        case ft::WRITE:
            r.close();
            break;
        case ft::CLOSE:
            qianjs::event_loop::end_operation();
            resolve_void(ctx->ph);
            defer_release_write_req(ctx);
            break;
        default:
            break;
        }
    });

    qianjs::event_loop::begin_operation();
    req->open(path, flags, mode);
    return ph->toValue();
}

qjs::Value fsWriteFileAsync(qjs::Engine& engine, std::string path, std::vector<uint8_t> data) {
    const file_flags flags = file_flags::WRONLY | file_flags::CREAT | file_flags::TRUNC;
#ifdef _WIN32
    const int mode = _S_IREAD | _S_IWRITE;
#else
    const int mode = 0644;
#endif
    return write_file_impl(engine, std::move(path), std::move(data), flags, mode);
}

qjs::Value fsMkdirAsync(qjs::Engine& engine, std::string path, bool recursive) {
    auto ph = qianjs::promise::create_shared(engine);
    if (!ph) {
        return {};
    }

    if (recursive) {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path), ec);
        if (ec) {
            reject(ph, ec.message());
            return ph->toValue();
        }
        resolve_void(ph);
        return ph->toValue();
    }

    struct MkdirAsyncCtx {
        std::shared_ptr<qjs::Promise> ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
    };
    auto ctx = std::make_shared<MkdirAsyncCtx>();
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
        if (ev.type == ft::MKDIR) {
            qianjs::event_loop::end_operation();
            resolve_void(ctx->ph);
            qianjs::event_loop::defer([ctx](qjs::Engine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    req->mkdir(path, 0777);
    return ph->toValue();
}
