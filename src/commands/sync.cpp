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
    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << "Usage: makekeeper sync" << std::endl;
        std::cout << std::endl;
        std::cout << "Download and sync the package catalog from the remote repository." << std::endl;
        return 0;
    }

    if (cli::reject_unknown_options(args, "Usage: makekeeper sync")) {
        return 1;
    }

    std::cout << "Downloading package catalog..." << std::endl;

    // Download packages.json to temp location first
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
        std::cout << "Package list is already up to date (version " << remote_version << ")" << std::endl;
        std::remove(temp_file.c_str());
        return 0;
    }

    std::cout << "Fetching package versions..." << std::endl;

    // Enrich packages with version information
    for (auto& item : j["packages"]) {
        std::string url = item["url"];
        std::string version = utils::get_latest_tag(url);
        item["latest_version"] = version;
    }

    std::remove(temp_file.c_str());

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
