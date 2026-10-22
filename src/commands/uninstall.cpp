#include "uninstall.h"
#include "../cli.h"
#include "../registry.h"
#include "../utils.h"
#include "../config.h"
#include "../colors.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace makekeeper {
namespace commands {

int uninstall(const std::vector<std::string>& args) {
    const char* usage = "Usage: makekeeper uninstall <NAME> [--yes]";

    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << usage << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  NAME    Package name to uninstall" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -y, --yes    Don't ask for confirmation" << std::endl;
        return 0;
    }

    std::string name;
    bool yes = false;

    // Parse args: the package name and --yes in any order, rejecting anything unknown
    for (const auto& arg : args) {
        if (arg == "--yes" || arg == "-y") {
            yes = true;
        } else if (cli::reject_unknown_options({arg}, usage)) {
            return 1;
        } else if (name.empty()) {
            name = arg;
        } else {
            std::cerr << "Error: Unexpected argument '" << arg << "'." << std::endl;
            std::cerr << usage << std::endl;
            return 1;
        }
    }

    if (name.empty()) {
        std::cerr << usage << std::endl;
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

    // Stop now if root can't be had; otherwise the files would stay while the clone was deleted
    if (!utils::can_get_root()) {
        return 1;
    }

    // MakeKeeper deletes the clone it made for this package; directories installed from are left alone
    bool legacy_source = utils::is_legacy_source(record->source_dir);
    bool managed_source = legacy_source ||
        fs::path(record->source_dir).parent_path() == fs::path(utils::user_sources_dir());
    bool has_source = !record->source_dir.empty() && fs::exists(record->source_dir);

    // Show what is about to be removed and ask first, unless --yes was given
    if (!yes) {
        std::ostringstream summary;
        std::vector<std::string> present;
        for (const auto& list : {record->files, record->symlinks}) {
            for (const auto& path : list) {
                bool listed = std::find(present.begin(), present.end(), path) != present.end();
                if (!listed && (fs::exists(path) || utils::is_symlink(path))) {
                    present.push_back(path);
                }
            }
        }

        summary << std::endl;
        summary << "Package:  " << colors::green << name << colors::reset << std::endl;
        if (has_source) {
            summary << "Runs:     make uninstall as root" << std::endl;
        }
        if (present.empty()) {
            summary << "Removes:  " << colors::gray << "no tracked files are present" << colors::reset << std::endl;
        }
        for (size_t i = 0; i < present.size(); i++) {
            summary << (i == 0 ? "Removes:  " : "          ") << present[i] << std::endl;
        }
        if (managed_source && has_source) {
            summary << "Deletes:  " << record->source_dir << std::endl;
        }
        if (name == "makekeeper") {
            summary << "Deletes:  " << config::REGISTRY_DIR << " (the registry and package catalog)" << std::endl;
        }
        summary << std::endl;
        if (!cli::confirm(summary.str(), "Uninstall " + name + "?")) {
            return 1;
        }
        std::cout << std::endl;
    }

    std::cout << "Uninstalling " << name << "..." << std::endl;

    // Run make uninstall if source dir exists
    if (has_source) {
        int result = utils::run_helper({"make", record->source_dir, "uninstall"});
        // pkexec exits with 126 or 127 when permission wasn't given; stop before removing anything
        if (result == 126 || result == 127) {
            std::cerr << "Error: Didn't get permission to uninstall " << name << ". Nothing was removed." << std::endl;
            return 1;
        }
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

    // Delete the clone MakeKeeper made for this package
    if (legacy_source) {
        if (utils::run_helper({"remove-source", fs::path(record->source_dir).filename().string()}) == 0) {
            std::cout << "  Removed source: " << record->source_dir << std::endl;
        }
    } else if (managed_source) {
        std::error_code ec;
        fs::remove_all(record->source_dir, ec);
        if (!ec) {
            std::cout << "  Removed source: " << record->source_dir << std::endl;
        }
    }

    // Remove from registry
    if (!registry.remove(name)) {
        std::cerr << "Error: Removed " << name << "'s files, but couldn't update the registry." << std::endl;
        return 1;
    }

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
