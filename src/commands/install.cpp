#include "install.h"
#include "../cli.h"
#include "../registry.h"
#include "../utils.h"
#include "../config.h"
#include "../colors.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fs = std::filesystem;

namespace makekeeper {
namespace commands {

int install(const std::vector<std::string>& args) {
    const char* usage = "Usage: makekeeper install <NAME|URL|DIR> [--name NAME] [--override] [--yes]";

    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << usage << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  NAME|URL|DIR    Package name, Git URL, or local directory" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --name NAME    Specify custom package name" << std::endl;
        std::cout << "  --override     Reinstall if already installed" << std::endl;
        std::cout << "  -y, --yes      Don't ask for confirmation" << std::endl;
        return 0;
    }

    std::string source;
    std::string name;
    bool override = false;
    bool yes = false;

    // Parse args: one source and options in any order, rejecting anything unknown rather than silently ignoring it
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--name") {
            if (i + 1 >= args.size()) {
                std::cerr << "Error: --name needs a value." << std::endl;
                return 1;
            }
            name = args[i + 1];
            i++;
        } else if (args[i] == "--override") {
            override = true;
        } else if (args[i] == "--yes" || args[i] == "-y") {
            yes = true;
        } else if (cli::reject_unknown_options({args[i]}, usage)) {
            return 1;
        } else if (source.empty()) {
            source = args[i];
        } else {
            std::cerr << "Error: Unexpected argument '" << args[i] << "'." << std::endl;
            std::cerr << usage << std::endl;
            return 1;
        }
    }

    if (source.empty()) {
        std::cerr << usage << std::endl;
        return 1;
    }

    // Load registry
    auto& registry = registry::Registry::instance();
    if (!registry.load()) {
        std::cerr << "Error: Failed to load registry." << std::endl;
        return 1;
    }

    // Early check: if name is provided and already installed without --override, fail early
    if (!name.empty() && registry.exists(name) && !override) {
        std::cerr << "Error: " << name << " is already installed. Use --override to reinstall." << std::endl;
        return 1;
    }

    // Early check: if source is a package name (not URL or dir) and already installed
    bool is_url = (source.find("://") != std::string::npos || source.find("git@") == 0);
    bool is_dir = fs::exists(source);
    
    if (!is_url && !is_dir && registry.exists(source) && !override) {
        std::cerr << "Error: " << source << " is already installed. Use --override to reinstall." << std::endl;
        return 1;
    }

    // Determine if source is a URL, directory, or package name
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
        source_dir = utils::user_sources_dir() + "/" + name;
    } else if (fs::exists(source)) {
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
        
        // Check if already installed after determining name from directory
        if (registry.exists(name) && !override) {
            std::cerr << "Error: " << name << " is already installed. Use --override to reinstall." << std::endl;
            return 1;
        }
    } else {
        // It's a package name - look up in catalog
        std::ifstream catalog_file(config::CACHE_FILE);
        if (catalog_file.is_open()) {
            try {
                nlohmann::json j;
                catalog_file >> j;
                
                if (j.contains("packages")) {
                    for (const auto& item : j["packages"]) {
                        if (item["name"] == source) {
                            url = item["url"];
                            if (name.empty()) {
                                name = source;
                            }
                            source_dir = utils::user_sources_dir() + "/" + name;
                            std::cout << "Found package in catalog: " << name << std::endl;
                            break;
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to parse catalog, treating as directory path" << std::endl;
            }
        }
        
        // If not found in catalog, report error
        if (url.empty()) {
            std::cerr << "Error: Package '" << source << "' not found in catalog." << std::endl;
            std::cerr << "Run 'makekeeper sync' to update the catalog, or provide a URL or directory path." << std::endl;
            return 1;
        }
    }

    // Don't install with empty name
    if (name.empty()) {
        std::cerr << "Error: Could not determine package name. Use --name to specify." << std::endl;
        return 1;
    }

    // Cloned packages are stored in a directory named after the package
    if (!url.empty() && (name == "." || name == ".." || name.find('/') != std::string::npos)) {
        std::cerr << "Error: Invalid package name '" << name << "'. Use --name to specify." << std::endl;
        return 1;
    }

    // Stop now if root can't be had, rather than after asking and building
    if (!utils::can_get_root()) {
        return 1;
    }

    // Show what is about to happen and ask first, unless --yes was given
    if (!yes) {
        std::ostringstream summary;
        summary << std::endl;
        summary << "Package:  " << colors::green << name << colors::reset << std::endl;
        summary << "Source:   " << colors::cyan << (url.empty() ? source_dir : url) << colors::reset << std::endl;
        summary << "Runs:     make, then make install as root" << std::endl;
        if (registry.exists(name)) {
            summary << "Replaces: the installed " << name << ", removing its tracked files first" << std::endl;
        }
        summary << std::endl;
        summary << "Only install software you trust: its Makefile runs with root privileges." << std::endl;
        if (!cli::confirm(summary.str(), "Install " + name + "?")) {
            return 1;
        }
        std::cout << std::endl;
    }

    // Check if already installed with --override (after name is determined)
    if (registry.exists(name)) {
        if (override) {
            std::cout << name << " is already installed. Overriding..." << std::endl;
            // Uninstall first with privilege
            auto existing = registry.get(name);
            if (existing) {
                // Remove tracked files and symlinks with privilege
                for (const auto& file : existing->files) {
                    if (fs::exists(file) || utils::is_symlink(file)) {
                        utils::run_helper({"remove", file});
                    }
                }
                for (const auto& symlink : existing->symlinks) {
                    if (fs::exists(symlink) || utils::is_symlink(symlink)) {
                        utils::run_helper({"remove", symlink});
                    }
                }
                // Delete a root-owned clone left in the cache by an older version
                if (utils::is_legacy_source(existing->source_dir)) {
                    utils::run_helper({"remove-source", fs::path(existing->source_dir).filename().string()});
                }
                // Remove from registry
                registry.remove(name);
                registry.save();
            }
        }
    }

    std::cout << "Installing " << name << "..." << std::endl;

    // Clone if URL, as the user so their SSH keys and Git credentials work
    if (!url.empty()) {
        std::cout << "Cloning repository into " << source_dir << "..." << std::endl;
        if (!utils::clone_source(url, source_dir)) {
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
        result = utils::exec_as_user("make", {"-C", source_dir});
        if (result != 0) {
            std::cerr << "Error: Build failed." << std::endl;
            return 1;
        }
    } else {
        std::cout << "Already built, skipping..." << std::endl;
    }

    // Install
    std::cout << "Installing..." << std::endl;
    result = utils::run_helper({"make", source_dir, "install"});
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
        commit_hash = utils::git_query(source_dir, {"rev-parse", "HEAD"});

        // Get remote URL if not already set
        if (url.empty()) {
            git_url = utils::git_query(source_dir, {"remote", "get-url", "origin"});
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

    // Track installed files from common locations (add regardless, install succeeded)
    std::string binary_path = std::string(config::BIN_DIR) + "/" + name;
    record.files.push_back(binary_path);
    record.symlinks.push_back(binary_path);

    // Track manpage
    std::string manpage_path = std::string(config::MAN_DIR) + "/" + name + ".1";
    record.files.push_back(manpage_path);

    // Track fish completion
    std::string fish_comp_path = "/usr/share/fish/vendor_completions.d/" + name + ".fish";
    record.files.push_back(fish_comp_path);

    // Track desktop file
    std::string desktop_path = std::string(config::DESKTOP_DIR) + "/" + name + ".desktop";
    record.files.push_back(desktop_path);

    // Track icons
    std::string icon_png = std::string(config::ICON_DIR) + "/" + name + "-128.png";
    record.files.push_back(icon_png);

    std::string icon_svg = std::string(config::ICON_SVG_DIR) + "/" + name + ".svg";
    record.files.push_back(icon_svg);

    registry.add(record);

    std::cout << "✓ Installed " << name << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
