#include "native/process/process_module.h"

#include "runtime/instance.h"
#include "runtime/runtime_context.h"

#include <js_engine.h>
#include <js_module.h>
#include <js_types.h>
#include <quickjs.h>

#include <filesystem>
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

JSValue argv_to_js(JSContext* c, const std::vector<std::string>& argv) {
    JSValue arr = JS_NewArray(c);
    if (JS_IsException(arr)) {
        return arr;
    }
    for (uint32_t i = 0; i < argv.size(); i++) {
        JSValue s = JS_NewString(c, argv[i].c_str());
        if (JS_IsException(s) || JS_SetPropertyUint32(c, arr, i, s) < 0) {
            JS_FreeValue(c, arr);
            return JS_EXCEPTION;
        }
    }
    return arr;
}

JSValue env_to_js(JSContext* c, const std::vector<std::pair<std::string, std::string>>& env) {
    JSValue obj = JS_NewObject(c);
    if (JS_IsException(obj)) {
        return obj;
    }
    for (const auto& kv : env) {
        JSValue v = JS_NewString(c, kv.second.c_str());
        if (JS_IsException(v) || JS_SetPropertyStr(c, obj, kv.first.c_str(), v) < 0) {
            JS_FreeValue(c, obj);
            return JS_EXCEPTION;
        }
    }
    return obj;
}

} // namespace

const char* ProcessPlugin::name() const {
    return "process";
}

void ProcessPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    (void)engine;
    auto& m = root.module("process");

    m.funcDynamic("pid", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        return JS_NewInt32(c, current_pid());
    });

    m.funcDynamic("platform", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        return JS_NewString(c, platform_id());
    });

    m.funcDynamic("cwd", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        std::string path = current_working_directory();
        return JS_NewString(c, path.c_str());
    });

    m.funcDynamic("argv", 0, 0, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        qianjs::RuntimeContext* runtime = current_host();
        if (!runtime) {
            return JS_NewArray(c);
        }
        return argv_to_js(c, runtime->argv);
    });

    m.funcDynamic("env", 0, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        qianjs::RuntimeContext* runtime = current_host();
        if (!runtime) {
            return JS_UNDEFINED;
        }
        if (argc == 0) {
            return env_to_js(c, runtime->env);
        }

        bool ok = false;
        std::string key = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok) {
            return JS_EXCEPTION;
        }
        for (const auto& kv : runtime->env) {
            if (kv.first == key) {
                return JS_NewString(c, kv.second.c_str());
            }
        }
        return JS_UNDEFINED;
    });

    m.func("getExitCode", []() -> int {
        qianjs::RuntimeContext* runtime = current_host();
        return runtime ? runtime->exit_code : 0;
    });

    m.func("exitCode", []() -> int {
        qianjs::RuntimeContext* runtime = current_host();
        return runtime ? runtime->exit_code : 0;
    });

    m.func("setExitCode", [](int code) {
        if (qianjs::RuntimeContext* runtime = current_host()) {
            runtime->exit_code = code;
        }
    });
}
