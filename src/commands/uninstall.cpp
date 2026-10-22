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
    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << "Usage: makekeeper uninstall <NAME>" << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  NAME    Package name to uninstall" << std::endl;
        return 0;
    }

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
        utils::run_helper({"make", record->source_dir, "uninstall"});
    }

    // Remove all installed files with privilege
    for (const auto& file : record->files) {
        if (fs::exists(file) || utils::is_symlink(file)) {
            if (utils::run_helper({"remove", file}) == 0) {
                std::cout << "  Removed: " << file << std::endl;
            } else {
                std::cerr << "  Warning: Failed to remove " << file << std::endl;
            }
        }
    }

    // Remove symlinks with privilege
    for (const auto& symlink : record->symlinks) {
        if (utils::is_symlink(symlink)) {
            if (utils::run_helper({"remove", symlink}) == 0) {
                std::cout << "  Removed symlink: " << symlink << std::endl;
            } else {
                std::cerr << "  Warning: Failed to remove " << symlink << std::endl;
            }
        }
    }

    // Delete the clone MakeKeeper made for this package; directories installed from are left alone
    if (utils::is_legacy_source(record->source_dir)) {
        if (utils::run_helper({"remove-source", fs::path(record->source_dir).filename().string()}) == 0) {
            std::cout << "  Removed source: " << record->source_dir << std::endl;
        }
    } else if (fs::path(record->source_dir).parent_path() == fs::path(utils::user_sources_dir())) {
        std::error_code ec;
        fs::remove_all(record->source_dir, ec);
        if (!ec) {
            std::cout << "  Removed source: " << record->source_dir << std::endl;
        }
    }

    // Remove from registry
    registry.remove(name);

    // If uninstalling makekeeper itself, remove registry and cache
    if (name == "makekeeper") {
        std::cout << "  Removing registry..." << std::endl;
        utils::run_helper({"purge"});
    }

    std::cout << "✓ Uninstalled " << name << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
