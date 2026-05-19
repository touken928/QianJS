#pragma once

#include <js_engine.h>
#include <quickjs.h>

#include "native/default_plugins.h"
#include "runtime/instance.h"
#include "runtime/runtime_context.h"

#include <string>
#include <utility>
#include <vector>

namespace qianjs::test {

/** One test run with plugins, active scheduler, and proper shutdown. */
struct TestRuntime {
    RuntimeInstance instance;

    TestRuntime(std::vector<std::string> argv,
                std::vector<std::pair<std::string, std::string>> env = {}) {
        instance.initialize(defaultPlugins());
        instance.host().argv = std::move(argv);
        instance.host().env = std::move(env);
        instance.begin_script_execution();
    }

    qjs::JSEngine& engine() { return instance.engine(); }
    void drain() { instance.run_until_idle(); }
};

inline bool run_module(qjs::JSEngine& engine, const std::string& virtual_name, const std::string& code) {
    return engine.runModuleCode(virtual_name, code);
}

inline std::string global_string(JSContext* c, const char* prop) {
    JSValue g = JS_GetGlobalObject(c);
    JSValue v = JS_GetPropertyStr(c, g, prop);
    JS_FreeValue(c, g);
    if (JS_IsUndefined(v) || JS_IsNull(v) || JS_IsUninitialized(v)) {
        JS_FreeValue(c, v);
        return {};
    }
    const char* s = JS_ToCString(c, v);
    std::string out = s ? s : "";
    if (s) {
        JS_FreeCString(c, s);
    }
    JS_FreeValue(c, v);
    return out;
}

inline int global_int(JSContext* c, const char* prop, bool* ok = nullptr) {
    JSValue g = JS_GetGlobalObject(c);
    JSValue v = JS_GetPropertyStr(c, g, prop);
    JS_FreeValue(c, g);
    int32_t n = 0;
    int r = JS_ToInt32(c, &n, v);
    JS_FreeValue(c, v);
    if (ok) {
        *ok = (r == 0);
    }
    return static_cast<int>(n);
}

/** Double-quoted JS string literal (safe to embed in generated module source). */
inline std::string js_string_literal(const std::string& s) {
    std::string o;
    o.push_back('"');
    for (unsigned char u : s) {
        if (u == '"' || u == '\\') {
            o.push_back('\\');
            o.push_back(static_cast<char>(u));
        } else if (u == '\n') {
            o += "\\n";
        } else if (u == '\r') {
            o += "\\r";
        } else if (u == '\t') {
            o += "\\t";
        } else if (u < 0x20u) {
            static const char hex[] = "0123456789abcdef";
            o += "\\u00";
            o.push_back(hex[(u >> 4) & 0xfu]);
            o.push_back(hex[u & 0xfu]);
        } else {
            o.push_back(static_cast<char>(u));
        }
    }
    o.push_back('"');
    return o;
}

} // namespace qianjs::test
