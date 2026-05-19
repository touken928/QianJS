#pragma once

#include <filesystem>

namespace qianjs::cli {

/** Compile `input_path` to `./dist/<stem>.qbc`. Returns process exit code. */
int build_command(const std::filesystem::path& input_path);

} // namespace qianjs::cli
