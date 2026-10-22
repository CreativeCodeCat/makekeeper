#include "search.h"
#include "../cli.h"
#include "../config.h"
#include "../utils.h"
#include "../colors.h"
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
    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << "Usage: makekeeper search <QUERY>" << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  QUERY    Search term to find packages" << std::endl;
        std::cout << std::endl;
        std::cout << "Searches through package names and descriptions." << std::endl;
        return 0;
    }

    if (cli::reject_unknown_options(args, "Usage: makekeeper search <QUERY>")) {
        return 1;
    }

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
                    std::cout << "Search results for '" << colors::yellow << query << colors::reset << "':" << std::endl;
                    std::cout << std::endl;
                    found = true;
                }
                std::cout << "  " << colors::green << pkg.name << colors::reset << " " << colors::cyan << pkg.latest_version << colors::reset << std::endl;
                std::cout << "    " << pkg.description << std::endl;
                std::cout << "    Category: " << colors::gray << pkg.category << colors::reset << std::endl;
                std::cout << std::endl;
            }
        }

        if (!found) {
            std::cout << "No packages found matching '" << colors::yellow << query << colors::reset << "'" << std::endl;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse package cache." << std::endl;
        return 1;
    }
}

} // namespace commands
} // namespace makekeeper
