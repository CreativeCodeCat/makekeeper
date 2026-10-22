#include "info.h"
#include "../cli.h"
#include "../registry.h"
#include "../config.h"
#include <iostream>
#include <iomanip>

namespace makekeeper {
namespace commands {

int info(const std::vector<std::string>& args) {
    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << "Usage: makekeeper info <NAME>" << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  NAME    Package name to show info for" << std::endl;
        std::cout << std::endl;
        std::cout << "Shows detailed information about an installed package." << std::endl;
        return 0;
    }

    if (cli::reject_unknown_options(args, "Usage: makekeeper info <NAME>")) {
        return 1;
    }

    if (args.empty()) {
        std::cerr << "Usage: makekeeper info <NAME>" << std::endl;
        return 1;
    }

    std::string name = args[0];

    // Load registry
    auto& registry = registry::Registry::instance();
    if (!registry.load()) {
        std::cerr << "Error: Failed to load registry." << std::endl;
        return 1;
    }

    // Check if installed
    auto record = registry.get(name);
    if (!record) {
        std::cerr << "Error: " << name << " is not installed." << std::endl;
        return 1;
    }

    // Display package info
    std::cout << "Package: " << record->name << std::endl;
    std::cout << std::endl;
    
    std::cout << "URL: " << (record->url.empty() ? "N/A" : record->url) << std::endl;
    std::cout << "Source Directory: " << record->source_dir << std::endl;
    
    if (!record->commit_hash.empty()) {
        std::cout << "Commit: " << record->commit_hash << std::endl;
    }
    
    std::cout << "Installed: " << record->install_date << std::endl;
    std::cout << std::endl;
    
    std::cout << "Files (" << record->files.size() << "):" << std::endl;
    for (const auto& file : record->files) {
        std::cout << "  " << file << std::endl;
    }
    
    if (!record->symlinks.empty()) {
        std::cout << std::endl;
        std::cout << "Symlinks (" << record->symlinks.size() << "):" << std::endl;
        for (const auto& symlink : record->symlinks) {
            std::cout << "  " << symlink << std::endl;
        }
    }

    return 0;
}

} // namespace commands
} // namespace makekeeper
