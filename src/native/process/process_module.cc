#include "native/process/process_module.h"

#include "runtime/instance.h"
#include "runtime/runtime_context.h"

#include <qjs/call.h>
#include <qjs/engine.h>
#include <qjs/module.h>

#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {

namespace fs_ns = std::filesystem;

static int current_pid() {
#ifdef _WIN32
    return static_cast<int>(_getpid());
#else
    return static_cast<int>(::getpid());
#endif
}

static const char* platform_id() {
#ifdef _WIN32
    return "win32";
#elif defined(__APPLE__)
    return "darwin";
#else
    return "linux";
#endif
}

static std::string current_working_directory() {
    std::error_code ec;
    fs_ns::path p = fs_ns::current_path(ec);
    if (ec) {
        return {};
    }
    return p.string();
}

qianjs::RuntimeContext* current_host() {
    if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
        return &inst->host();
    }
    return nullptr;
}

} // namespace

const char* ProcessPlugin::name() const {
    return "process";
}

void ProcessPlugin::install(qjs::Context&, qjs::Module& root) {
    auto& m = root.module("process");

    m.funcDynamic("pid", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        return qjs::Result<qjs::Value>::ok(ctx.engine().int64(current_pid()));
    });

    m.funcDynamic("platform", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        return qjs::Result<qjs::Value>::ok(ctx.engine().string(platform_id()));
    });

    m.funcDynamic("cwd", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        return qjs::Result<qjs::Value>::ok(ctx.engine().string(current_working_directory()));
    });

    m.funcDynamic("argv", 0, 0, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        qianjs::RuntimeContext* runtime = current_host();
        auto arr = ctx.engine().array();
        if (runtime) {
            for (const std::string& s : runtime->argv) {
                arr.pushString(s);
            }
        }
        return qjs::Result<qjs::Value>::ok(arr.build());
    });

    m.funcDynamic("env", 0, 1, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        qianjs::RuntimeContext* runtime = current_host();
        if (!runtime) {
            return qjs::Result<qjs::Value>::ok(ctx.undefined());
        }
        if (ctx.argc() == 0) {
            auto obj = ctx.engine().object();
            for (const auto& kv : runtime->env) {
                obj.setString(kv.first, kv.second);
            }
            return qjs::Result<qjs::Value>::ok(obj.build());
        }

        auto key = ctx.stringArg(0);
        if (!key.success) {
            return qjs::Result<qjs::Value>::fail(key.error);
        }
        for (const auto& kv : runtime->env) {
            if (kv.first == key.value) {
                return qjs::Result<qjs::Value>::ok(ctx.engine().string(kv.second));
            }
        }
        return qjs::Result<qjs::Value>::ok(ctx.undefined());
    });

    m.func("getExitCode", []() -> int {
        qianjs::RuntimeContext* runtime = current_host();
        return runtime ? runtime->exit_code : 0;
    });

    m.func("exitCode", []() -> int {
        qianjs::RuntimeContext* runtime = current_host();
        return runtime ? runtime->exit_code : 0;
    });

    m.func("setExitCode", std::function<void(int)>([](int code) {
        if (qianjs::RuntimeContext* runtime = current_host()) {
            runtime->exit_code = code;
        }
    }));
}
