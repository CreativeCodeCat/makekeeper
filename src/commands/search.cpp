#include "search.h"
#include "../config.h"
#include "../utils.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>

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

int search(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: makekeeper search <QUERY>" << std::endl;
        return 1;
    }

    std::string query = args[0];

    // Load packages.json
    std::ifstream file(config::CACHE_FILE);
    if (!file.is_open()) {
        std::cerr << "Error: Package cache not found. Run 'makekeeper sync' first." << std::endl;
        return 1;
    }

    try {
        json j;
        file >> j;

        std::vector<Package> packages;
        for (const auto& item : j["packages"]) {
            Package pkg;
            pkg.name = item["name"];
            pkg.url = item["url"];
            pkg.description = item["description"];
            pkg.category = item["category"];
            if (item.contains("latest_version")) {
                pkg.latest_version = item["latest_version"];
            } else {
                pkg.latest_version = "latest";
            }
            packages.push_back(pkg);
        }

        // Search
        std::string query_lower = query;
        std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);

        bool found = false;
        for (const auto& pkg : packages) {
            std::string name_lower = pkg.name;
            std::string desc_lower = pkg.description;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            std::transform(desc_lower.begin(), desc_lower.end(), desc_lower.begin(), ::tolower);

            if (name_lower.find(query_lower) != std::string::npos ||
                desc_lower.find(query_lower) != std::string::npos) {
                if (!found) {
                    std::cout << "Search results for '" << query << "':" << std::endl;
                    std::cout << std::endl;
                    found = true;
                }
                std::cout << "  " << pkg.name << " v" << pkg.latest_version << std::endl;
                std::cout << "    " << pkg.description << std::endl;
                std::cout << "    Category: " << pkg.category << std::endl;
                std::cout << std::endl;
            }
        }

        if (!found) {
            std::cout << "No packages found matching '" << query << "'" << std::endl;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse package cache." << std::endl;
        return 1;
    }
}

} // namespace commands
} // namespace makekeeper
