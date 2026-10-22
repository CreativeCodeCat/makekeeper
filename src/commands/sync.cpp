#include "sync.h"
#include "../cli.h"
#include "../config.h"
#include "../utils.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace makekeeper {
namespace commands {

struct Package {
    std::string name;
    std::string url;
    std::string description;
    std::string category;
    std::string latest_version;
};

int sync(const std::vector<std::string>& args) {
    const char* usage = "Usage: makekeeper sync [--check]";

    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << usage << std::endl;
        std::cout << std::endl;
        std::cout << "Download and sync the package catalog from the remote repository." << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --check    Only report whether a newer catalog is available; don't replace it" << std::endl;
        return 0;
    }

    bool check_only = false;
    for (const auto& arg : args) {
        if (arg == "--check") {
            check_only = true;
        } else if (cli::reject_unknown_options({arg}, usage)) {
            return 1;
        }
    }

    if (!check_only) {
        std::cout << "Downloading package catalog..." << std::endl;
    }

    // Download packages.json to temp location first. This is read-only and needs no privilege,
    // whether or not we end up replacing the cached catalog.
    std::string temp_file = "/tmp/packages.json.tmp";
    if (!utils::download_file(config::PACKAGES_URL, temp_file)) {
        std::cerr << "Error: Failed to download package catalog." << std::endl;
        return 1;
    }

    // Load the downloaded JSON
    std::ifstream temp(temp_file);
    if (!temp.is_open()) {
        std::cerr << "Error: Failed to open downloaded catalog." << std::endl;
        return 1;
    }

    json j;
    try {
        temp >> j;
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse package catalog." << std::endl;
        return 1;
    }
    temp.close();
    std::remove(temp_file.c_str());

    // Check version
    std::string remote_version = j["version"];
    std::string local_version = "0.0.0";

    std::ifstream local_file(config::CACHE_FILE);
    if (local_file.is_open()) {
        try {
            json local_j;
            local_file >> local_j;
            local_version = local_j["version"];
        } catch (...) {
            // Ignore parse errors
        }
        local_file.close();
    }

    if (remote_version == local_version) {
        if (check_only) {
            std::cout << "up-to-date " << remote_version << std::endl;
        } else {
            std::cout << "Package list is already up to date (version " << remote_version << ")" << std::endl;
        }
        return 0;
    }

    if (check_only) {
        std::cout << "update-available " << remote_version << std::endl;
        return 0;
    }

    std::cout << "Fetching package versions..." << std::endl;

    // Enrich packages with version information
    for (auto& item : j["packages"]) {
        std::string url = item["url"];
        std::string version = utils::get_latest_tag(url);
        item["latest_version"] = version;
    }

    // The catalog lives in /var/lib, so the helper writes it as root
    if (utils::run_helper({"write-catalog"}, j.dump(2)) != 0) {
        std::cerr << "Error: Failed to save package catalog." << std::endl;
        return 1;
    }

    std::cout << "✓ Package catalog updated to version " << remote_version << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
