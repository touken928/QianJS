#include "native/fs/fs_module.h"

#include "native/fs/fs_sync.h"
#include "native/fs/fs_uv.h"

#include <qjs/engine.h>
#include <qjs/module.h>
#include <qjs/value.h>

#include <functional>
#include <string>
#include <vector>

const char* FsPlugin::name() const {
    return "fs";
}

void FsPlugin::install(qjs::Context& ctx, qjs::Module& root) {
    qjs::Engine& eng = ctx.engine();
    auto& m = root.module("fs");

    auto one_path = [&eng](auto fn) {
        return std::function<qjs::Value(std::string)>(
            [&eng, fn](std::string path) -> qjs::Value { return fn(eng, std::move(path)); });
    };

    m.func("readFile", one_path([](qjs::Engine& e, std::string p) { return fsReadFileAsync(e, std::move(p), false); }));
    m.func("readFileBytes", one_path([](qjs::Engine& e, std::string p) { return fsReadFileAsync(e, std::move(p), true); }));

    m.func("writeFile", std::function<qjs::Value(std::string, qjs::Value)>([&eng](std::string path, qjs::Value data) -> qjs::Value {
        std::vector<uint8_t> bytes;
        if (auto b = data.toBytes(); b.success) {
            bytes = std::move(b.value);
        } else if (data.isString()) {
            auto asStr = data.toString();
            if (!asStr.success) {
                return eng.throwTypeError("writeFile: invalid string data");
            }
            bytes.assign(asStr.value.begin(), asStr.value.end());
        } else {
            return eng.throwTypeError("writeFile: data must be string, ArrayBuffer, or TypedArray");
        }
        return fsWriteFileAsync(eng, std::move(path), std::move(bytes));
    }));

    m.func("mkdir", one_path([](qjs::Engine& e, std::string p) { return fsMkdirAsync(e, std::move(p), false); }));
    m.func("mkdirRecursive", one_path([](qjs::Engine& e, std::string p) { return fsMkdirAsync(e, std::move(p), true); }));
    m.func("readdir", one_path([](qjs::Engine& e, std::string p) { return fsReaddirAsync(e, std::move(p)); }));
    m.func("stat", one_path([](qjs::Engine& e, std::string p) { return fsStatAsync(e, std::move(p)); }));
    m.func("unlink", one_path([](qjs::Engine& e, std::string p) { return fsUnlinkAsync(e, std::move(p)); }));
    m.func("rmdir", one_path([](qjs::Engine& e, std::string p) { return fsRmdirAsync(e, std::move(p)); }));

    install_fs_sync(eng, m.module("sync"));
}
