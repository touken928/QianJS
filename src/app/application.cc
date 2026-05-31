#include "app/application.h"

#include <qianjs_modules.h>

#include "native/default_plugins.h"
#include "runtime/embed.h"
#include "runtime/app_host.h"
#include "runtime/instance.h"
#include "runtime/runtime_context.h"

#include <iostream>
#include <string>
#include <utility>

namespace qianjs {

int Application::run_script(const std::filesystem::path& input_path, std::vector<std::string> argv) {
    RuntimeInstance instance;
    instance.initialize(defaultPlugins());

    if (argv.empty()) {
        instance.host().argv.push_back(input_path.string());
    } else {
        instance.host().argv = std::move(argv);
    }
    instance.host().env = captureEnvironment();

    bool ok = false;
    if (input_path.extension() == ".qbc") {
        std::vector<uint8_t> bytecode = Embed::readBinaryFile(input_path);
        if (bytecode.empty()) {
            std::cerr << "Error: Cannot read bytecode: " << input_path << std::endl;
            return 1;
        }
        ok = instance.run_bytecode(bytecode.data(), bytecode.size());
    } else {
        ok = instance.run_file(input_path);
    }

    if (!ok) {
        instance.shutdown();
        return 1;
    }

#if QIANJS_MODULE_GAME
    FrameLoopOptions frame_opts;
    if (instance.host().argv.size() > 1) {
        try {
            const int64_t n = std::stoll(instance.host().argv.back());
            if (n > 0) {
                frame_opts.max_frames = n;
            }
        } catch (const std::exception&) {
        }
    }
    instance.try_run_deferred_app(frame_opts);
#endif

    instance.run_until_idle();
    const int code = instance.host().exit_code;
    instance.shutdown();
    return code;
}

int Application::run_embedded(std::vector<std::string> argv) {
    std::vector<uint8_t> embedded = Embed::readEmbeddedBytecode();
    if (embedded.empty()) {
        return -1;
    }

    RuntimeInstance instance;
    instance.initialize(defaultPlugins());
    instance.host().argv = std::move(argv);
    instance.host().env = captureEnvironment();

    if (!instance.run_bytecode(embedded.data(), embedded.size())) {
        instance.shutdown();
        return 1;
    }

    instance.run_until_idle();
    const int code = instance.host().exit_code;
    instance.shutdown();
    return code;
}

} // namespace qianjs
