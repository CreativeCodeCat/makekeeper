#include "list.h"
#include "../registry.h"
#include "../config.h"
#include <iostream>
#include <iomanip>

namespace makekeeper {
namespace commands {

int list(const std::vector<std::string>& args) {
    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << "Usage: makekeeper list" << std::endl;
        std::cout << std::endl;
        std::cout << "List all installed packages with their details." << std::endl;
        return 0;
    }

    // Load registry
    auto& registry = registry::Registry::instance();
    if (!registry.load()) {
        std::cerr << "Error: Failed to load registry." << std::endl;
        return 1;
    }

    auto records = registry.list();

    // Filter out records with empty names
    std::vector<registry::Record> valid_records;
    for (const auto& record : records) {
        if (!record.name.empty()) {
            valid_records.push_back(record);
        }
    }

    if (valid_records.empty()) {
        std::cout << "No packages installed." << std::endl;
        return 0;
    }

    std::cout << "Installed packages:" << std::endl;
    std::cout << std::endl;

    for (const auto& record : valid_records) {
        std::cout << "  " << record.name << std::endl;
        std::cout << "    URL: " << (record.url.empty() ? "N/A" : record.url) << std::endl;
        std::cout << "    Installed: " << record.install_date << std::endl;
        if (!record.commit_hash.empty()) {
            std::cout << "    Commit: " << record.commit_hash.substr(0, 7) << std::endl;
        }
        std::cout << std::endl;
    }

    return 0;
}

} // namespace commands
} // namespace makekeeper
