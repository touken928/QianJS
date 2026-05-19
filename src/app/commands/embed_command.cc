#include "app/commands/embed_command.h"

#include "runtime/embed.h"

#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace qianjs::cli {

int embed_command(const fs::path& qbc_path) {
    if (!fs::exists(qbc_path)) {
        std::cerr << "Error: File not found: " << qbc_path << std::endl;
        return 1;
    }

    if (qbc_path.extension() != ".qbc") {
        std::cerr << "Error: Expected .qbc file, got: " << qbc_path << std::endl;
        return 1;
    }

    std::vector<uint8_t> bytecode = Embed::readBinaryFile(qbc_path);
    if (bytecode.empty()) {
        std::cerr << "Error: Cannot read bytecode file: " << qbc_path << std::endl;
        return 1;
    }

    fs::path output_path = qbc_path.parent_path() / qbc_path.stem();
#ifdef _WIN32
    output_path.replace_extension(".exe");
#endif

    if (!Embed::createEmbeddedExecutable(bytecode, output_path)) {
        std::cerr << "Error: Cannot create embedded executable: " << output_path << std::endl;
        return 1;
    }

    const auto output_size = fs::file_size(output_path);
    std::cout << "Embedded: " << qbc_path.string() << " -> " << output_path.string() << " (" << output_size
              << " bytes)" << std::endl;
    return 0;
}

} // namespace qianjs::cli
