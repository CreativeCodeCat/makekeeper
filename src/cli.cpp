#include "cli.h"
#include "config.h"
#include <iostream>
#include <algorithm>

namespace makekeeper {
namespace cli {

CLI& CLI::instance() {
    static CLI instance;
    return instance;
}

void CLI::register_command(const Command& command) {
    commands_.push_back(command);
}

int CLI::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "--help" || cmd == "-h") {
        print_help();
        return 0;
    }

    if (cmd == "--version" || cmd == "-v") {
        print_version();
        return 0;
    }

    std::vector<std::string> args;
    for (int i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }

    for (const auto& command : commands_) {
        if (command.name == cmd) {
            return command.handler(args);
        }
    }

    std::cerr << "Unknown command: " << cmd << std::endl;
    print_help();
    return 1;
}

void CLI::print_help() const {
    std::cout << "MakeKeeper v" << config::VERSION << " - Safer installer for Makefile-based applications" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: makekeeper <COMMAND> [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;

    // Find max command name length for alignment
    size_t max_name_length = 0;
    for (const auto& command : commands_) {
        if (command.name.length() > max_name_length) {
            max_name_length = command.name.length();
        }
    }

    for (const auto& command : commands_) {
        std::cout << "  " << command.name;
        // Pad with spaces to align descriptions
        for (size_t i = command.name.length(); i < max_name_length + 4; i++) {
            std::cout << " ";
        }
        std::cout << command.description << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help     Show this help message" << std::endl;
    std::cout << "  -v, --version  Show version information" << std::endl;
}

void CLI::print_version() const {
    std::cout << "MakeKeeper v" << config::VERSION << std::endl;
}

} // namespace cli
} // namespace makekeeper
