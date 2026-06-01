#include "native/process/process_module.h"

#include "runtime/instance.h"
#include "runtime/runtime_context.h"

#include <qjs/engine.h>
#include <qjs/module.h>
#include <qjs/value.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

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

void ProcessPlugin::install(qjs::Context& ctx, qjs::Module& root) {
    qjs::Engine& eng = ctx.engine();
    auto& m = root.module("process");

    m.func("pid", std::function<int64_t()>([]() -> int64_t { return current_pid(); }));
    m.func("platform", std::function<std::string()>([]() -> std::string { return platform_id(); }));
    m.func("cwd", std::function<std::string()>([]() -> std::string { return current_working_directory(); }));

    m.func("argv", std::function<qjs::Value()>([&eng]() -> qjs::Value {
        qianjs::RuntimeContext* runtime = current_host();
        auto arr = eng.array();
        if (runtime) {
            for (const std::string& s : runtime->argv) {
                arr.push(s);
            }
        }
        return arr.build();
    }));

    m.func("env", std::function<qjs::Value(std::optional<std::string>)>([&eng](std::optional<std::string> key) -> qjs::Value {
        qianjs::RuntimeContext* runtime = current_host();
        if (!runtime) {
            return eng.undefined();
        }
        if (!key) {
            auto obj = eng.object();
            for (const auto& kv : runtime->env) {
                obj.set(kv.first, kv.second);
            }
            return obj.build();
        }
        for (const auto& kv : runtime->env) {
            if (kv.first == *key) {
                return eng.string(kv.second);
            }
        }
        return eng.undefined();
    }));

    m.func("getExitCode", std::function<int()>([]() -> int {
        qianjs::RuntimeContext* runtime = current_host();
        return runtime ? runtime->exit_code : 0;
    }));

    m.func("exitCode", std::function<int()>([]() -> int {
        qianjs::RuntimeContext* runtime = current_host();
        return runtime ? runtime->exit_code : 0;
    }));

    m.func("setExitCode", std::function<void(int)>([](int code) {
        if (qianjs::RuntimeContext* runtime = current_host()) {
            runtime->exit_code = code;
        }
    }));
}
