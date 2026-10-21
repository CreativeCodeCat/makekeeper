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

    // Check if root
    if (!utils::is_root()) {
        std::cerr << "Error: This command requires root privileges." << std::endl;
        return 1;
    }

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
        std::string uninstall_cmd = "cd " + record->source_dir + " && make uninstall";
        system(uninstall_cmd.c_str());
    }

    // Remove symlinks
    for (const auto& symlink : record->symlinks) {
        if (utils::is_symlink(symlink)) {
            utils::remove_symlink(symlink);
        }
    }

    // Remove from registry
    registry.remove(name);

    std::cout << "✓ Uninstalled " << name << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
