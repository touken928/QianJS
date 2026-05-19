#include "app/cli.h"

#include "app/cli_usage.h"
#include "app/commands/build_command.h"
#include "app/commands/embed_command.h"
#include "app/commands/run_command.h"

#include <iostream>
#include <string>
#include <vector>

namespace qianjs::cli {

namespace {

int dispatch(int argc, char* argv[]) {
    if (argc < 2) {
        const int bundled = run_bundled_command(argc, argv);
        if (bundled >= 0) {
            return bundled;
        }
        print_usage(argv[0]);
        return 1;
    }

    const std::string cmd = argv[1];

    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        print_usage(argv[0]);
        return 0;
    }

    if (cmd == "build") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file\n"
                      << "Usage: " << argv[0] << " build <file.js>" << std::endl;
            return 1;
        }
        return build_command(argv[2]);
    }

    if (cmd == "embed") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file\n"
                      << "Usage: " << argv[0] << " embed <file.qbc>" << std::endl;
            return 1;
        }
        return embed_command(argv[2]);
    }

    if (cmd == "run") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file\n"
                      << "Usage: " << argv[0] << " run <file.js|file.qbc> [args...]" << std::endl;
            return 1;
        }
        std::vector<std::string> script_argv;
        script_argv.reserve(static_cast<size_t>(argc - 2));
        for (int i = 2; i < argc; ++i) {
            script_argv.emplace_back(argv[i]);
        }
        return run_command(argv[2], std::move(script_argv));
    }

    std::cerr << "Error: Unknown command: " << cmd << std::endl;
    print_usage(argv[0]);
    return 1;
}

} // namespace

} // namespace qianjs::cli

int qianjs_cli_run(int argc, char* argv[]) {
    return qianjs::cli::dispatch(argc, argv);
}
