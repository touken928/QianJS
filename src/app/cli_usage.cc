#include "app/cli_usage.h"

#include <qianjs_modules.h>

#include <iostream>

namespace qianjs::cli {

void print_usage(const char* prog_name) {
    std::cout << "QianJS — JavaScript runtime\n\n"
              << "Usage:\n"
              << "  " << prog_name << " run <file.js|qbc> [args...]   Run JS or bytecode\n"
              << "  " << prog_name << " build <file.js>     Compile JS to ./dist/<name>.qbc\n"
              << "  " << prog_name << " embed <file.qbc>    Embed bytecode into a standalone executable\n"
              << "  " << prog_name << " help                Show this help\n"
              << "\nBuild profile: " << QIANJS_BUILD_PROFILE << "  modules: " << QIANJS_BUILD_MODULES << "\n"
              << std::endl;
}

} // namespace qianjs::cli
