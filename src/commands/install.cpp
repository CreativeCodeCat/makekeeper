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
        std::cerr << "Usage: makekeeper install <NAME|URL> [--name NAME] [--dir DIR] [--override]" << std::endl;
        return 1;
    }

    std::string source = args[0];
    std::string name;
    std::string dir = ".";
    bool override = false;

    // Parse args
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--name" && i + 1 < args.size()) {
            name = args[i + 1];
            i++;
        } else if (args[i] == "--dir" && i + 1 < args.size()) {
            dir = args[i + 1];
            i++;
        } else if (args[i] == "--override") {
            override = true;
        }
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
            // Handle "." or empty directory name
            if (name == "." || name.empty()) {
                // Use parent directory name
                name = fs::path(source_dir).parent_path().filename().string();
                if (name.empty() || name == ".") {
                    name = "makekeeper";  // Final fallback
                }
            }
        }
    }

    // Check if already installed
    if (registry.exists(name)) {
        if (override) {
            std::cout << name << " is already installed. Overriding..." << std::endl;
            // Uninstall first
            auto existing = registry.get(name);
            if (existing) {
                // Remove files
                for (const auto& file : existing->files) {
                    if (fs::exists(file)) {
                        fs::remove(file);
                    }
                }
                // Remove symlinks
                for (const auto& symlink : existing->symlinks) {
                    if (fs::exists(symlink)) {
                        fs::remove(symlink);
                    }
                }
                // Remove from registry
                registry.remove(name);
                registry.save();
            }
        } else {
            std::cerr << "Error: " << name << " is already installed. Use --override to reinstall." << std::endl;
            return 1;
        }
    }

    // Don't install with empty name
    if (name.empty()) {
        std::cerr << "Error: Could not determine package name. Use --name to specify." << std::endl;
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

    // Check if already built (check for common build artifacts)
    bool already_built = false;
    if (fs::exists(source_dir + "/build") || fs::exists(source_dir + "/target") || fs::exists(source_dir + "/bin")) {
        already_built = true;
    }

    // Build only if not already built
    int result;
    if (!already_built) {
        std::cout << "Building..." << std::endl;
        result = utils::exec_command_with_privilege("make", {"-C", source_dir});
        if (result != 0) {
            std::cerr << "Error: Build failed." << std::endl;
            return 1;
        }
    } else {
        std::cout << "Already built, skipping..." << std::endl;
    }

    // Install
    std::cout << "Installing..." << std::endl;
    result = utils::exec_command_with_privilege("make", {"-C", source_dir, "install"});
    if (result != 0) {
        std::cerr << "Error: Install failed." << std::endl;
        return 1;
    }

    // Get commit hash and URL if git repo
    std::string commit_hash;
    std::string git_url;

    // Check if source directory is a git repository
    if (fs::exists(source_dir + "/.git")) {
        // Get commit hash
        commit_hash = utils::exec_command("git", {"-C", source_dir, "rev-parse", "HEAD"});
        if (!commit_hash.empty() && commit_hash.back() == '\n') {
            commit_hash.pop_back();
        }

        // Get remote URL if not already set
        if (url.empty()) {
            git_url = utils::exec_command("git", {"-C", source_dir, "remote", "get-url", "origin"});
            if (!git_url.empty() && git_url.back() == '\n') {
                git_url.pop_back();
            }
        } else {
            git_url = url;
        }
    } else if (!url.empty()) {
        // URL was provided, use it
        git_url = url;
    }

    // Create record
    registry::Record record;
    record.name = name;
    record.url = git_url;
    record.source_dir = source_dir;
    record.commit_hash = commit_hash;
    record.install_date = utils::get_timestamp();

    // Track installed files from common locations
    std::string binary_path = std::string(config::BIN_DIR) + "/" + name;
    if (fs::exists(binary_path)) {
        record.files.push_back(binary_path);
        record.symlinks.push_back(binary_path);
    }

    // Track manpage if it exists
    std::string manpage_path = std::string(config::MAN_DIR) + "/" + name + ".1";
    if (fs::exists(manpage_path)) {
        record.files.push_back(manpage_path);
    }

    // Track fish completion if it exists
    std::string fish_comp_path = "/usr/share/fish/vendor_completions.d/" + name + ".fish";
    if (fs::exists(fish_comp_path)) {
        record.files.push_back(fish_comp_path);
    }

    // Track desktop file if it exists
    std::string desktop_path = std::string(config::DESKTOP_DIR) + "/" + name + ".desktop";
    if (fs::exists(desktop_path)) {
        record.files.push_back(desktop_path);
    }

    // Track icons if they exist
    std::string icon_png = std::string(config::ICON_DIR) + "/" + name + "-128.png";
    if (fs::exists(icon_png)) {
        record.files.push_back(icon_png);
    }

    std::string icon_svg = std::string(config::ICON_SVG_DIR) + "/" + name + ".svg";
    if (fs::exists(icon_svg)) {
        record.files.push_back(icon_svg);
    }

    registry.add(record);

    std::cout << "✓ Installed " << name << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
