#include "fetch.h"
#include "../cli.h"
#include "../registry.h"
#include "../config.h"
#include <iostream>
#include <iomanip>

namespace makekeeper {
namespace commands {

int fetch(const std::vector<std::string>& args) {
    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << "Usage: makekeeper fetch" << std::endl;
        std::cout << std::endl;
        std::cout << "Show package statistics: total installed / packages with Git repositories." << std::endl;
        return 0;
    }

    if (cli::reject_unknown_options(args, "Usage: makekeeper fetch")) {
        return 1;
    }

    // Load registry
    auto& registry = registry::Registry::instance();
    if (!registry.load()) {
        std::cout << "0/0 (makekeeper)" << std::endl;
        return 0;
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
        std::cout << "0/0 (makekeeper)" << std::endl;
        return 0;
    }

    int total_installed = valid_records.size();
    int git_repos = 0;

    for (const auto& record : valid_records) {
        if (!record.url.empty()) {
            git_repos++;
        }
    }

    std::cout << total_installed << "/" << git_repos << " (makekeeper)" << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
