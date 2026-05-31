#include "native/fs/fs_module.h"

#include "native/fs/fs_sync.h"
#include "native/fs/fs_uv.h"

#include <qjs/call.h>
#include <qjs/engine.h>
#include <qjs/module.h>

#include <string>
#include <vector>

const char* FsPlugin::name() const {
    return "fs";
}

void FsPlugin::install(qjs::Context& ctx, qjs::Module& root) {
    qjs::Engine* eng = &ctx.engine();
    auto& m = root.module("fs");

    auto one_path = [eng](auto fn) {
        return [eng, fn](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
            auto path = ctx.stringArg(0);
            if (!path.success) {
                return qjs::Result<qjs::Value>::fail(path.error);
            }
            return qjs::Result<qjs::Value>::ok(fn(*eng, std::move(path.value)));
        };
    };

    m.funcDynamic("readFile", 1, 1, one_path([](qjs::Engine& e, std::string p) {
        return fsReadFileAsync(e, std::move(p), false);
    }));

    m.funcDynamic("readFileBytes", 1, 1, one_path([](qjs::Engine& e, std::string p) {
        return fsReadFileAsync(e, std::move(p), true);
    }));

    m.funcDynamic("writeFile", 2, 2, [eng](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        auto path = ctx.stringArg(0);
        if (!path.success) {
            return qjs::Result<qjs::Value>::fail(path.error);
        }
        std::vector<uint8_t> data;
        auto arg = ctx.valueArg(1);
        if (!arg.success) {
            return qjs::Result<qjs::Value>::fail(arg.error);
        }
        if (auto bytes = arg.value.toBytes(); bytes.success) {
            data = std::move(bytes.value);
        } else if (arg.value.isString()) {
            auto asStr = arg.value.toString();
            if (!asStr.success) {
                return qjs::Result<qjs::Value>::fail(asStr.error);
            }
            data.assign(asStr.value.begin(), asStr.value.end());
        } else {
            return qjs::Result<qjs::Value>::ok(ctx.throwTypeError(
                "writeFile: data must be string, ArrayBuffer, or TypedArray"));
        }
        return qjs::Result<qjs::Value>::ok(fsWriteFileAsync(*eng, std::move(path.value), std::move(data)));
    });

    m.funcDynamic("mkdir", 1, 1, one_path([](qjs::Engine& e, std::string p) {
        return fsMkdirAsync(e, std::move(p), false);
    }));

    m.funcDynamic("mkdirRecursive", 1, 1, one_path([](qjs::Engine& e, std::string p) {
        return fsMkdirAsync(e, std::move(p), true);
    }));

    m.funcDynamic("readdir", 1, 1, one_path([](qjs::Engine& e, std::string p) {
        return fsReaddirAsync(e, std::move(p));
    }));

    m.funcDynamic("stat", 1, 1, one_path([](qjs::Engine& e, std::string p) {
        return fsStatAsync(e, std::move(p));
    }));

    m.funcDynamic("unlink", 1, 1, one_path([](qjs::Engine& e, std::string p) {
        return fsUnlinkAsync(e, std::move(p));
    }));

    m.funcDynamic("rmdir", 1, 1, one_path([](qjs::Engine& e, std::string p) {
        return fsRmdirAsync(e, std::move(p));
    }));

    install_fs_sync(m.module("sync"));
}
