#include "native/console/console_module.h"

#include <qjs/call.h>
#include <qjs/module.h>

#include <iostream>
#include <string>

namespace {

qjs::Result<qjs::Value> write_line(std::ostream& os, qjs::CallContext& ctx) {
    std::string out;
    out.reserve(static_cast<size_t>(ctx.argc()) * 8u);
    for (int i = 0; i < ctx.argc(); i++) {
        if (i > 0) {
            out.push_back(' ');
        }
        auto v = ctx.valueArg(i);
        if (!v.success) {
            return v;
        }
        auto s = v.value.toString();
        if (!s.success) {
            return qjs::Result<qjs::Value>::fail(s.error);
        }
        out += s.value;
    }
    os << out << '\n';
    return qjs::Result<qjs::Value>::ok(ctx.undefined());
}

} // namespace

const char* ConsolePlugin::name() const {
    return "console";
}

void ConsolePlugin::install(qjs::Context&, qjs::Module& root) {
    auto& c = root.module("console");

    c.funcDynamic("log", 0, 32, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        return write_line(std::cout, ctx);
    });
    c.funcDynamic("info", 0, 32, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        return write_line(std::cout, ctx);
    });
    c.funcDynamic("debug", 0, 32, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        return write_line(std::cout, ctx);
    });
    c.funcDynamic("warn", 0, 32, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        return write_line(std::cerr, ctx);
    });
    c.funcDynamic("error", 0, 32, [](qjs::CallContext& ctx) -> qjs::Result<qjs::Value> {
        return write_line(std::cerr, ctx);
    });
}
