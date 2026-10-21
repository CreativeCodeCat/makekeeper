#include "list.h"
#include "../registry.h"
#include "../config.h"
#include <iostream>
#include <iomanip>

namespace makekeeper {
namespace commands {

int list(const std::vector<std::string>& args) {
    // Load registry
    auto& registry = registry::Registry::instance();
    if (!registry.load()) {
        std::cerr << "Error: Failed to load registry." << std::endl;
        return 1;
    }

    auto records = registry.list();

    if (records.empty()) {
        std::cout << "No packages installed." << std::endl;
        return 0;
    }

    std::cout << "Installed packages:" << std::endl;
    std::cout << std::endl;

    for (const auto& record : records) {
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
