#include "native/app/app_module.h"

#include "runtime/app_host.h"
#include "runtime/instance.h"

#include <js_engine.h>
#include <js_module.h>
#include <quickjs.h>

#include <cstring>

namespace {

bool read_opt_int(JSContext* c, JSValue opts, const char* key, int* out) {
    JSValue v = JS_GetPropertyStr(c, opts, key);
    if (JS_IsUndefined(v)) {
        return true;
    }
    int32_t n = 0;
    const int r = JS_ToInt32(c, &n, v);
    JS_FreeValue(c, v);
    if (r != 0) {
        return false;
    }
    *out = static_cast<int>(n);
    return true;
}

bool read_opt_i64(JSContext* c, JSValue opts, const char* key, int64_t* out) {
    JSValue v = JS_GetPropertyStr(c, opts, key);
    if (JS_IsUndefined(v)) {
        return true;
    }
    if (JS_ToInt64(c, out, v) != 0) {
        JS_FreeValue(c, v);
        return false;
    }
    JS_FreeValue(c, v);
    return true;
}

bool read_opt_double(JSContext* c, JSValue opts, const char* key, double* out) {
    JSValue v = JS_GetPropertyStr(c, opts, key);
    if (JS_IsUndefined(v)) {
        return true;
    }
    if (JS_ToFloat64(c, out, v) != 0) {
        JS_FreeValue(c, v);
        return false;
    }
    JS_FreeValue(c, v);
    return true;
}

bool read_opt_string(JSContext* c, JSValue opts, const char* key, std::string* out) {
    JSValue v = JS_GetPropertyStr(c, opts, key);
    if (JS_IsUndefined(v)) {
        return true;
    }
    const char* s = JS_ToCString(c, v);
    JS_FreeValue(c, v);
    if (!s) {
        return false;
    }
    *out = s;
    JS_FreeCString(c, s);
    return true;
}

bool parse_options(JSContext* c, int argc, JSValue* argv, qianjs::FrameLoopOptions& opts, std::string& title_storage) {
    if (argc < 2 || JS_IsUndefined(argv[1])) {
        return true;
    }
    if (!JS_IsObject(argv[1])) {
        return false;
    }
    if (!read_opt_int(c, argv[1], "width", &opts.width)) {
        return false;
    }
    if (!read_opt_int(c, argv[1], "height", &opts.height)) {
        return false;
    }
    if (!read_opt_i64(c, argv[1], "maxFrames", &opts.max_frames)) {
        return false;
    }
    if (!read_opt_double(c, argv[1], "fps", &opts.target_fps)) {
        return false;
    }
    if (!read_opt_double(c, argv[1], "fixedStep", &opts.fixed_dt)) {
        return false;
    }
    if (!read_opt_string(c, argv[1], "title", &title_storage)) {
        return false;
    }
    if (!title_storage.empty()) {
        opts.title = title_storage.c_str();
    }
    return true;
}

} // namespace

const char* AppPlugin::name() const {
    return "app";
}

void AppPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    (void)engine;
    auto& m = root.module("app");

    m.funcDynamic("createApp", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!JS_IsObject(argv[0])) {
            return JS_ThrowTypeError(c, "createApp: expected object with update and render");
        }
        JSValue update = JS_GetPropertyStr(c, argv[0], "update");
        JSValue render = JS_GetPropertyStr(c, argv[0], "render");
        const bool ok = JS_IsFunction(c, update) && JS_IsFunction(c, render);
        JS_FreeValue(c, update);
        JS_FreeValue(c, render);
        if (!ok) {
            return JS_ThrowTypeError(c, "createApp: update and render must be functions");
        }
        if (qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current()) {
            inst->set_deferred_app(c, argv[0]);
        }
        return JS_DupValue(c, argv[0]);
    });

    m.funcDynamic("runApp", 1, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
        if (!inst) {
            return JS_ThrowInternalError(c, "runApp: no active runtime instance");
        }

        inst->clear_deferred_app();

        qianjs::AppHost host;
        if (!host.load_from_object(c, argv[0])) {
            return JS_ThrowTypeError(c, "runApp: app must provide update and render functions");
        }

        qianjs::FrameLoopOptions opts;
        std::string title;
        if (!parse_options(c, argc, argv, opts, title)) {
            host.release(c);
            return JS_EXCEPTION;
        }

        if (!inst->run_frame_loop(host, opts)) {
            host.release(c);
            return JS_EXCEPTION;
        }
        return JS_UNDEFINED;
    });
}
