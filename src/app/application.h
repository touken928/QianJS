#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace qianjs {

/** Single entry for `qianjs run` / embed: owns one RuntimeInstance per invocation. */
class Application {
public:
    static int run_script(const std::filesystem::path& input_path, std::vector<std::string> argv = {});
    static int run_embedded(std::vector<std::string> argv = {});
};

} // namespace qianjs
