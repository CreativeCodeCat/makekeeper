#include "sync.h"
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
    // Check if root
    if (!utils::is_root()) {
        std::cerr << "Error: This command requires root privileges." << std::endl;
        return 1;
    }

    // Ensure cache directory exists
    utils::ensure_directory(std::string(config::CACHE_DIR));

    std::cout << "Downloading package catalog..." << std::endl;

    // Download packages.json
    std::string temp_file = std::string(config::CACHE_DIR) + "/packages.json.tmp";
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
        fs::remove(temp_file);
        return 0;
    }

    std::cout << "Fetching package versions..." << std::endl;

    // Enrich packages with version information
    for (auto& item : j["packages"]) {
        std::string url = item["url"];
        std::string version = utils::get_latest_tag(url);
        item["latest_version"] = version;
    }

    // Write enriched catalog
    std::ofstream output(config::CACHE_FILE);
    if (!output.is_open()) {
        std::cerr << "Error: Failed to write package catalog." << std::endl;
        return 1;
    }

    output << j.dump(2);
    output.close();

    // Remove temp file
    fs::remove(temp_file);

    std::cout << "✓ Package catalog updated to version " << remote_version << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
