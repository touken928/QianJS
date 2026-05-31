#include "native/fs/fs_sync.h"

#include "native/fs/fs_stat_js.h"

#include "runtime/event_loop/event_loop.h"

#include <qjs/call.h>
#include <qjs/engine.h>
#include <qjs/module.h>

#include <climits>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <uv.h>

namespace {

namespace fs = std::filesystem;

/** Single read via size preflight (avoids istream iterator byte-by-byte). */
std::optional<std::vector<uint8_t>> read_entire_file_bin(const fs::path& path) {
    std::error_code ec;
    const auto sz_u = fs::file_size(path, ec);
    if (ec)
        return std::nullopt;
    if (sz_u > static_cast<std::uintmax_t>(SIZE_MAX))
        return std::nullopt;
    const std::size_t sz = static_cast<std::size_t>(sz_u);

    std::vector<uint8_t> out(sz);
    if (sz == 0)
        return out;

    std::ifstream in(path, std::ios::binary);
    if (!in)
        return std::nullopt;
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(sz));
    if (!in || static_cast<std::size_t>(in.gcount()) != sz)
        return std::nullopt;
    return out;
}

qjs::Result<qjs::Value> fail_msg(std::string message) {
    return qjs::Result<qjs::Value>::fail(qjs::ErrorInfo{std::move(message), {}, {}});
}

qjs::Result<qjs::Value> fail_ec(const char* prefix, const std::error_code& ec) {
    std::string msg = prefix;
    msg += ec.message();
    return fail_msg(std::move(msg));
}

auto one_path = [](auto fn) {
    return [fn](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto path = ctx.stringArg(0);
        if (!path.success) {
            return qjs::Result<qjs::Value>::fail(path.error);
        }
        return fn(ctx, std::move(path.value));
    };
};

} // namespace

void install_fs_sync(qjs::Module& sync) {
    sync.funcDynamic("readFile", 1, 1, one_path([](qjs::CallContext& ctx, std::string pathStr) -> qjs::Result<qjs::Value> {
        const fs::path path(pathStr);
        const auto bytes = read_entire_file_bin(path);
        if (!bytes) {
            return fail_msg("readFile: cannot open or read file");
        }
        return qjs::Result<qjs::Value>::ok(
            ctx.engine().string(std::string(reinterpret_cast<const char*>(bytes->data()), bytes->size())));
    }));

    sync.funcDynamic("readFileBytes", 1, 1, one_path([](qjs::CallContext& ctx, std::string pathStr) -> qjs::Result<qjs::Value> {
        const fs::path path(pathStr);
        const auto bytes = read_entire_file_bin(path);
        if (!bytes) {
            return fail_msg("readFileBytes: cannot open or read file");
        }
        return qjs::Result<qjs::Value>::ok(ctx.engine().arrayBuffer(bytes->data(), bytes->size()));
    }));

    sync.funcDynamic("writeFile", 2, 2, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto path = ctx.stringArg(0);
        if (!path.success) {
            return qjs::Result<qjs::Value>::fail(path.error);
        }
        auto bytes = ctx.bytesArg(1);
        if (!bytes.success) {
            auto asStr = ctx.stringArg(1);
            if (!asStr.success) {
                return fail_msg("writeFile: data must be string, ArrayBuffer, or TypedArray");
            }
            bytes.value.assign(asStr.value.begin(), asStr.value.end());
        }
        std::ofstream out(path.value, std::ios::binary | std::ios::trunc);
        if (!out) {
            return fail_msg("writeFile: cannot open file for write");
        }
        if (!bytes.value.empty()) {
            out.write(reinterpret_cast<const char*>(bytes.value.data()),
                static_cast<std::streamsize>(bytes.value.size()));
        }
        if (!out) {
            return fail_msg("writeFile: write failed");
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    sync.funcDynamic("mkdir", 1, 1, one_path([](qjs::CallContext& ctx, std::string pathStr) -> qjs::Result<qjs::Value> {
        std::error_code ec;
        fs::create_directory(fs::path(pathStr), ec);
        if (ec) {
            return fail_ec("mkdir: ", ec);
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));

    sync.funcDynamic("mkdirRecursive", 1, 1, one_path([](qjs::CallContext& ctx, std::string pathStr) -> qjs::Result<qjs::Value> {
        std::error_code ec;
        fs::create_directories(fs::path(pathStr), ec);
        if (ec) {
            return fail_ec("mkdirRecursive: ", ec);
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));

    sync.funcDynamic("readdir", 1, 1, one_path([](qjs::CallContext& ctx, std::string pathStr) -> qjs::Result<qjs::Value> {
        std::error_code ec;
        const fs::path p(pathStr);
        const fs::file_status st = fs::status(p, ec);
        if (ec) {
            return fail_ec("readdir: ", ec);
        }
        if (!fs::is_directory(st)) {
            return fail_msg("readdir: not a directory");
        }
        auto arr = ctx.engine().array();
        for (const auto& ent : fs::directory_iterator(p, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                return fail_ec("readdir: ", ec);
            }
            arr.pushString(ent.path().filename().string());
        }
        return qjs::Result<qjs::Value>::ok(arr.build());
    }));

    sync.funcDynamic("stat", 1, 1, one_path([](qjs::CallContext& ctx, std::string path) -> qjs::Result<qjs::Value> {
        uv_fs_t req;
        uv_loop_t* lp = qianjs::event_loop::uv::loop();
        const int r = uv_fs_stat(lp, &req, path.c_str(), nullptr);
        if (r < 0) {
            std::string err = uv_strerror(r);
            uv_fs_req_cleanup(&req);
            return fail_msg(err);
        }
        qjs::Value o = fs_stat_to_value(ctx.engine(), req.statbuf);
        uv_fs_req_cleanup(&req);
        return qjs::Result<qjs::Value>::ok(std::move(o));
    }));

    sync.funcDynamic("unlink", 1, 1, one_path([](qjs::CallContext& ctx, std::string pathStr) -> qjs::Result<qjs::Value> {
        const fs::path p(pathStr);
        std::error_code ec;
        const auto sl = fs::symlink_status(p, ec);
        if (ec) {
            return fail_ec("unlink: ", ec);
        }
        if (fs::is_directory(sl) && !fs::is_symlink(sl)) {
            return fail_msg("unlink: path is a directory");
        }
        if (!fs::remove(p, ec)) {
            return fail_ec("unlink: ", ec);
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));

    sync.funcDynamic("rmdir", 1, 1, one_path([](qjs::CallContext& ctx, std::string pathStr) -> qjs::Result<qjs::Value> {
        const fs::path p(pathStr);
        std::error_code ec;
        const fs::file_status st = fs::status(p, ec);
        if (ec) {
            return fail_ec("rmdir: ", ec);
        }
        if (!fs::is_directory(st)) {
            return fail_msg("rmdir: not a directory");
        }
        if (!fs::is_empty(p, ec)) {
            return fail_ec("rmdir: ", ec);
        }
        if (!fs::remove(p, ec)) {
            return fail_ec("rmdir: ", ec);
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    }));
}
