#pragma once

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#if !defined(_WIN32)
extern "C" {
extern char** environ;
}
#endif

namespace qianjs {

struct RuntimeContext {
    std::vector<std::string> argv;
    std::vector<std::pair<std::string, std::string>> env;
    int exit_code = 0;
};

inline std::vector<std::pair<std::string, std::string>> captureEnvironment() {
    std::vector<std::pair<std::string, std::string>> out;
#if defined(_WIN32)
    char** envp = _environ;
#else
    char** envp = ::environ;
#endif
    if (!envp)
        return out;

    for (char** p = envp; *p; ++p) {
        const std::string kv = *p;
        const auto pos = kv.find('=');
        if (pos == std::string::npos)
            continue;
        out.emplace_back(kv.substr(0, pos), kv.substr(pos + 1));
    }
    return out;
}

} // namespace qianjs
