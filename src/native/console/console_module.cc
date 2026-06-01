#include "native/console/console_module.h"

#include <qjs/engine.h>
#include <qjs/module.h>
#include <qjs/value.h>

#include <iostream>
#include <string>
#include <vector>

namespace {

void write_line(std::ostream& os, const std::vector<qjs::Value>& args) {
    std::string out;
    out.reserve(args.size() * 8u);
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) {
            out.push_back(' ');
        }
        auto s = args[i].toString();
        if (!s.success) {
            continue;
        }
        out += s.value;
    }
    os << out << '\n';
}

} // namespace

const char* ConsolePlugin::name() const {
    return "console";
}

void ConsolePlugin::install(qjs::Context&, qjs::Module& root) {
    auto& c = root.module("console");
    auto log_fn = [](std::vector<qjs::Value> args) { write_line(std::cout, args); };
    c.func("log", log_fn);
    c.func("info", log_fn);
    c.func("debug", log_fn);
    c.func("warn", [](std::vector<qjs::Value> args) { write_line(std::cerr, args); });
    c.func("error", [](std::vector<qjs::Value> args) { write_line(std::cerr, args); });
}
