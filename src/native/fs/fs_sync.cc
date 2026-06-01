#include "native/fs/fs_sync.h"

#include "native/fs/fs_stat_js.h"

#include "runtime/event_loop/event_loop.h"

#include <qjs/engine.h>
#include <qjs/module.h>
#include <qjs/value.h>

#include <climits>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <uv.h>

namespace {

namespace fs = std::filesystem;

std::optional<std::vector<uint8_t>> read_entire_file_bin(const fs::path& path) {
    std::error_code ec;
    const auto sz_u = fs::file_size(path, ec);
    if (ec) {
        return std::nullopt;
    }
    if (sz_u > static_cast<std::uintmax_t>(SIZE_MAX)) {
        return std::nullopt;
    }
    const std::size_t sz = static_cast<std::size_t>(sz_u);

    std::vector<uint8_t> out(sz);
    if (sz == 0) {
        return out;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(sz));
    if (!in || static_cast<std::size_t>(in.gcount()) != sz) {
        return std::nullopt;
    }
    return out;
}

qjs::Value fail(qjs::Engine& eng, std::string message) {
    return eng.throwTypeError(message);
}

qjs::Value fail_ec(qjs::Engine& eng, const char* prefix, const std::error_code& ec) {
    std::string msg = prefix;
    msg += ec.message();
    return fail(eng, std::move(msg));
}

} // namespace

void install_fs_sync(qjs::Engine& eng, qjs::Module& sync) {
    sync.func("readFile", std::function<qjs::Value(std::string)>([&eng](std::string pathStr) -> qjs::Value {
        const fs::path path(pathStr);
        const auto bytes = read_entire_file_bin(path);
        if (!bytes) {
            return fail(eng, "readFile: cannot open or read file");
        }
        return eng.string(std::string(reinterpret_cast<const char*>(bytes->data()), bytes->size()));
    }));

    sync.func("readFileBytes", std::function<qjs::Value(std::string)>([&eng](std::string pathStr) -> qjs::Value {
        const fs::path path(pathStr);
        const auto bytes = read_entire_file_bin(path);
        if (!bytes) {
            return fail(eng, "readFileBytes: cannot open or read file");
        }
        return eng.arrayBuffer(bytes->data(), bytes->size());
    }));

    sync.func("writeFile", std::function<qjs::Value(std::string, qjs::Value)>([&eng](std::string path, qjs::Value data) -> qjs::Value {
        std::vector<uint8_t> bytes;
        if (auto b = data.toBytes(); b.success) {
            bytes = std::move(b.value);
        } else if (data.isString()) {
            auto asStr = data.toString();
            if (!asStr.success) {
                return fail(eng, "writeFile: invalid string data");
            }
            bytes.assign(asStr.value.begin(), asStr.value.end());
        } else {
            return fail(eng, "writeFile: data must be string, ArrayBuffer, or TypedArray");
        }
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) {
            return fail(eng, "writeFile: cannot open file for write");
        }
        if (!bytes.empty()) {
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }
        if (!out) {
            return fail(eng, "writeFile: write failed");
        }
        return eng.undefined();
    }));

    sync.func("mkdir", std::function<qjs::Value(std::string)>([&eng](std::string pathStr) -> qjs::Value {
        std::error_code ec;
        fs::create_directory(fs::path(pathStr), ec);
        if (ec) {
            return fail_ec(eng, "mkdir: ", ec);
        }
        return eng.undefined();
    }));

    sync.func("mkdirRecursive", std::function<qjs::Value(std::string)>([&eng](std::string pathStr) -> qjs::Value {
        std::error_code ec;
        fs::create_directories(fs::path(pathStr), ec);
        if (ec) {
            return fail_ec(eng, "mkdirRecursive: ", ec);
        }
        return eng.undefined();
    }));

    sync.func("readdir", std::function<qjs::Value(std::string)>([&eng](std::string pathStr) -> qjs::Value {
        std::error_code ec;
        const fs::path p(pathStr);
        const fs::file_status st = fs::status(p, ec);
        if (ec) {
            return fail_ec(eng, "readdir: ", ec);
        }
        if (!fs::is_directory(st)) {
            return fail(eng, "readdir: not a directory");
        }
        auto arr = eng.array();
        for (const auto& ent : fs::directory_iterator(p, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                return fail_ec(eng, "readdir: ", ec);
            }
            arr.push(ent.path().filename().string());
        }
        return arr.build();
    }));

    sync.func("stat", std::function<qjs::Value(std::string)>([&eng](std::string path) -> qjs::Value {
        uv_fs_t req;
        uv_loop_t* lp = qianjs::event_loop::uv::loop();
        const int r = uv_fs_stat(lp, &req, path.c_str(), nullptr);
        if (r < 0) {
            std::string err = uv_strerror(r);
            uv_fs_req_cleanup(&req);
            return fail(eng, err);
        }
        qjs::Value o = fs_stat_to_value(eng, req.statbuf);
        uv_fs_req_cleanup(&req);
        return o;
    }));

    sync.func("unlink", std::function<qjs::Value(std::string)>([&eng](std::string pathStr) -> qjs::Value {
        const fs::path p(pathStr);
        std::error_code ec;
        const auto sl = fs::symlink_status(p, ec);
        if (ec) {
            return fail_ec(eng, "unlink: ", ec);
        }
        if (fs::is_directory(sl) && !fs::is_symlink(sl)) {
            return fail(eng, "unlink: path is a directory");
        }
        if (!fs::remove(p, ec)) {
            return fail_ec(eng, "unlink: ", ec);
        }
        return eng.undefined();
    }));

    sync.func("rmdir", std::function<qjs::Value(std::string)>([&eng](std::string pathStr) -> qjs::Value {
        const fs::path p(pathStr);
        std::error_code ec;
        const fs::file_status st = fs::status(p, ec);
        if (ec) {
            return fail_ec(eng, "rmdir: ", ec);
        }
        if (!fs::is_directory(st)) {
            return fail(eng, "rmdir: not a directory");
        }
        if (!fs::is_empty(p, ec)) {
            return fail_ec(eng, "rmdir: ", ec);
        }
        if (!fs::remove(p, ec)) {
            return fail_ec(eng, "rmdir: ", ec);
        }
        return eng.undefined();
    }));
}
