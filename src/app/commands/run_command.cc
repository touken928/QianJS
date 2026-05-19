#include "app/commands/run_command.h"

#include "app/application.h"
#include "runtime/embed.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace qianjs::cli {

int run_command(const fs::path& input_path, std::vector<std::string> script_argv) {
    if (!fs::exists(input_path)) {
        std::cerr << "Error: File not found: " << input_path << std::endl;
        return 1;
    }
    if (script_argv.empty()) {
        script_argv.push_back(input_path.string());
    }
    return Application::run_script(input_path, std::move(script_argv));
}

int run_bundled_command(int argc, char* argv[]) {
    std::vector<std::string> script_argv;
    script_argv.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        script_argv.emplace_back(argv[i]);
    }

    const int embedded = Application::run_embedded(std::move(script_argv));
    if (embedded >= 0) {
        return embedded;
    }

    const fs::path exe_path = Embed::getExecutablePath();
    const fs::path exe_dir = exe_path.parent_path();
    const std::string exe_name = exe_path.stem().string();

    fs::path qbc_path = exe_dir / (exe_name + ".qbc");
    if (fs::exists(qbc_path)) {
        return run_command(qbc_path, {qbc_path.string()});
    }

    qbc_path = fs::path(exe_name + ".qbc");
    if (fs::exists(qbc_path)) {
        return run_command(qbc_path, {qbc_path.string()});
    }

    return -1;
}

} // namespace qianjs::cli
