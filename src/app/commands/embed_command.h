#pragma once

#include <filesystem>

namespace qianjs::cli {

/** Attach bytecode to a copy of the current executable. Returns process exit code. */
int embed_command(const std::filesystem::path& qbc_path);

} // namespace qianjs::cli
