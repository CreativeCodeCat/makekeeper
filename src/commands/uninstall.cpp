#include "uninstall.h"
#include "../registry.h"
#include "../utils.h"
#include "../config.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace makekeeper {
namespace commands {

int uninstall(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: makekeeper uninstall <NAME>" << std::endl;
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

    std::cout << "Uninstalling " << name << "..." << std::endl;

    // Run make uninstall if source dir exists
    if (!record->source_dir.empty() && fs::exists(record->source_dir)) {
        utils::exec_command_with_privilege("make", {"-C", record->source_dir, "uninstall"});
    }

    // Remove all installed files
    for (const auto& file : record->files) {
        if (fs::exists(file)) {
            try {
                // Use pkexec for files in system directories
                if (file.find("/usr/") == 0 || file.find("/var/") == 0) {
                    utils::exec_command_with_privilege("rm", {"-f", file});
                } else {
                    fs::remove(file);
                }
                std::cout << "  Removed: " << file << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "  Warning: Failed to remove " << file << ": " << e.what() << std::endl;
            }
        }
    }

    // Remove symlinks
    for (const auto& symlink : record->symlinks) {
        if (utils::is_symlink(symlink)) {
            utils::remove_symlink(symlink);
            std::cout << "  Removed symlink: " << symlink << std::endl;
        }
    }

    // Remove from registry
    registry.remove(name);

    // If uninstalling makekeeper itself, remove registry and cache
    if (name == "makekeeper") {
        std::cout << "  Removing registry..." << std::endl;
        utils::exec_command_with_privilege("rm", {"-rf", config::REGISTRY_DIR});
    }

    std::cout << "✓ Uninstalled " << name << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
