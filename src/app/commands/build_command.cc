#include "app/commands/build_command.h"

#include "native/default_plugins.h"
#include "runtime/embed.h"

#include <js_engine.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace qianjs::cli {

int build_command(const fs::path& input_path) {
    if (!fs::exists(input_path)) {
        std::cerr << "Error: File not found: " << input_path << std::endl;
        return 1;
    }

    std::string code = Embed::readTextFile(input_path);
    if (code.empty()) {
        std::cerr << "Error: Cannot read file: " << input_path << std::endl;
        return 1;
    }

    qjs::JSEngine engine;
    engine.initialize();
    defaultPlugins().installAll(engine, engine.root());
    qjs::JSEngine::CompileResult result = engine.compileModuleFromSource(code, input_path.string());
    engine.cleanup();
    if (!result.success) {
        std::cerr << "Compile error: " << result.error << std::endl;
        return 1;
    }

    fs::path output_path = fs::path("dist") / input_path.stem();
    output_path.replace_extension(".qbc");

    if (!Embed::writeBinaryFile(output_path, result.bytecode)) {
        std::cerr << "Error: Cannot write file: " << output_path << std::endl;
        return 1;
    }

    std::cout << "Compiled: " << input_path.string() << " -> " << output_path.string() << " ("
              << result.bytecode.size() << " bytes)" << std::endl;
    return 0;
}

} // namespace qianjs::cli
