#include "install.h"
#include "../registry.h"
#include "../utils.h"
#include "../config.h"
#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace makekeeper {
namespace commands {

int install(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: makekeeper install <NAME|URL> [--name NAME] [--dir DIR]" << std::endl;
        return 1;
    }

    std::string source = args[0];
    std::string name;
    std::string dir = ".";

    // Parse args
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--name" && i + 1 < args.size()) {
            name = args[i + 1];
            i++;
        } else if (args[i] == "--dir" && i + 1 < args.size()) {
            dir = args[i + 1];
            i++;
        }
    }

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

    // Determine if source is a URL or directory
    std::string url;
    std::string source_dir;

    if (source.find("://") != std::string::npos || source.find("git@") == 0) {
        // It's a URL
        url = source;
        if (name.empty()) {
            // Extract name from URL
            size_t last_slash = source.rfind('/');
            if (last_slash != std::string::npos) {
                name = source.substr(last_slash + 1);
                if (name.size() > 4 && name.substr(name.size() - 4) == ".git") {
                    name = name.substr(0, name.size() - 4);
                }
            } else {
                name = source;
            }
        }
        source_dir = std::string(config::CACHE_DIR) + "/" + name;
    } else {
        // It's a directory
        source_dir = fs::absolute(source).string();
        if (name.empty()) {
            name = fs::path(source_dir).filename().string();
        }
    }

    // Check if already installed
    if (registry.exists(name)) {
        std::cerr << "Error: " << name << " is already installed." << std::endl;
        return 1;
    }

    std::cout << "Installing " << name << "..." << std::endl;

    // Clone if URL
    if (!url.empty()) {
        std::cout << "Cloning repository..." << std::endl;
        std::string clone_cmd = "git clone " + url + " " + source_dir;
        int result = system(clone_cmd.c_str());
        if (result != 0) {
            std::cerr << "Error: Failed to clone repository." << std::endl;
            return 1;
        }
    }

    // Check for Makefile
    std::string makefile_path = source_dir + "/Makefile";
    if (!fs::exists(makefile_path)) {
        std::cerr << "Error: No Makefile found in " << source_dir << std::endl;
        return 1;
    }

    // Build
    std::cout << "Building..." << std::endl;
    std::string build_cmd = "cd " + source_dir + " && make";
    int result = system(build_cmd.c_str());
    if (result != 0) {
        std::cerr << "Error: Build failed." << std::endl;
        return 1;
    }

    // Install
    std::cout << "Installing..." << std::endl;
    std::string install_cmd = "cd " + source_dir + " && sudo make install";
    result = system(install_cmd.c_str());
    if (result != 0) {
        std::cerr << "Error: Install failed." << std::endl;
        return 1;
    }

    // Get commit hash if git repo
    std::string commit_hash;
    if (!url.empty()) {
        std::string hash_cmd = "cd " + source_dir + " && git rev-parse HEAD";
        commit_hash = utils::exec_command("git", {"-C", source_dir, "rev-parse", "HEAD"});
        // Remove trailing newline
        if (!commit_hash.empty() && commit_hash.back() == '\n') {
            commit_hash.pop_back();
        }
    }

    // Create record
    registry::Record record;
    record.name = name;
    record.url = url;
    record.source_dir = source_dir;
    record.commit_hash = commit_hash;
    record.install_date = utils::get_timestamp();

    // Scan for installed files (simplified)
    // In a real implementation, we'd track actual files installed

    registry.add(record);

    std::cout << "✓ Installed " << name << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
