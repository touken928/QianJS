#pragma once

#include <qjs/engine.h>

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
        instance.initialize();
        instance.host().argv = std::move(argv);
        instance.host().env = std::move(env);
        instance.begin_script_execution();
    }

    qjs::Engine& engine() { return instance.engine(); }
    void drain() { instance.run_until_idle(); }
};

inline bool run_module(qjs::Engine& engine, const std::string& virtual_name, const std::string& code) {
    return engine.evalModule(virtual_name, code).success;
}

inline std::string global_string(qjs::Engine& engine, const char* prop) {
    auto v = engine.getGlobal(prop);
    if (!v.success) {
        return {};
    }
    if (!v.value.valid() || v.value.isUndefined() || v.value.isNull()) {
        return {};
    }
    auto s = v.value.toString();
    if (!s.success) {
        return {};
    }
    if (s.value == "undefined") {
        return {};
    }
    return s.value;
}

inline int global_int(qjs::Engine& engine, const char* prop, bool* ok = nullptr) {
    auto v = engine.getGlobal(prop);
    if (!v.success) {
        if (ok) {
            *ok = false;
        }
        return 0;
    }
    if (v.value.isUndefined() || v.value.isNull()) {
        if (ok) {
            *ok = false;
        }
        return 0;
    }
    auto n = v.value.toInt32();
    if (ok) {
        *ok = n.success;
    }
    return n.success ? static_cast<int>(n.value) : 0;
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
