#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace qianjs::cli {

/** Run a `.js` or `.qbc` file via `Application::run_script`. */
int run_command(const std::filesystem::path& input_path, std::vector<std::string> script_argv);

/**
 * No subcommand: try embedded bytecode, then sibling `<exe>.qbc`.
 * Returns exit code, or `-1` if nothing to run.
 */
int run_bundled_command(int argc, char* argv[]);

} // namespace qianjs::cli
