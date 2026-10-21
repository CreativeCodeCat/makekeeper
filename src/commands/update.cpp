#include "update.h"
#include "../registry.h"
#include "../utils.h"
#include "../config.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace makekeeper {
namespace commands {

int update(const std::vector<std::string>& args) {
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

    // If args is empty, update all packages
    if (args.empty()) {
        std::cout << "Updating all packages..." << std::endl;
        std::cout << std::endl;

        int updated_count = 0;
        int skipped_count = 0;

        for (const auto& record : records) {
            if (record.url.empty()) {
                std::cout << record.name << ": Skipped (no URL)" << std::endl;
                skipped_count++;
                continue;
            }

            std::cout << "Updating " << record.name << "..." << std::endl;

            // Fetch changes
            int result = utils::exec_command_with_privilege("git", {"-C", record.source_dir, "fetch"});
            if (result != 0) {
                std::cerr << "  Error: Failed to fetch updates." << std::endl;
                continue;
            }

            // Check if there are changes
            std::string status = utils::exec_command("git", {"-C", record.source_dir, "status", "-sb"});

            // Check if ahead of origin
            if (status.find("behind") == std::string::npos) {
                std::cout << "  Already up to date." << std::endl;
                skipped_count++;
                continue;
            }

            // Pull changes
            result = utils::exec_command_with_privilege("git", {"-C", record.source_dir, "pull"});
            if (result != 0) {
                std::cerr << "  Error: Failed to pull updates." << std::endl;
                continue;
            }

            // Rebuild
            std::cout << "  Rebuilding..." << std::endl;
            result = utils::exec_command_with_privilege("make", {"-C", record.source_dir, "clean"});
            if (result != 0) {
                std::cerr << "  Error: Clean failed." << std::endl;
                continue;
            }
            result = utils::exec_command_with_privilege("make", {"-C", record.source_dir});
            if (result != 0) {
                std::cerr << "  Error: Build failed." << std::endl;
                continue;
            }

            // Reinstall
            std::cout << "  Reinstalling..." << std::endl;
            result = utils::exec_command_with_privilege("make", {"-C", record.source_dir, "install"});
            if (result != 0) {
                std::cerr << "  Error: Install failed." << std::endl;
                continue;
            }

            // Update commit hash
            std::string new_hash = utils::exec_command("git", {"-C", record.source_dir, "rev-parse", "HEAD"});
            if (!new_hash.empty() && new_hash.back() == '\n') {
                new_hash.pop_back();
            }

            registry::Record updated_record = record;
            updated_record.commit_hash = new_hash;
            registry.update(record.name, updated_record);

            std::cout << "  ✓ Updated " << record.name << std::endl;
            updated_count++;
        }

        std::cout << std::endl;
        std::cout << "Updated: " << updated_count << std::endl;
        std::cout << "Skipped: " << skipped_count << std::endl;

        return 0;
    }

    // Update specific package
    std::string name = args[0];

    // Check if installed
    auto record = registry.get(name);
    if (!record) {
        std::cerr << "Error: " << name << " is not installed." << std::endl;
        return 1;
    }

    if (record->url.empty()) {
        std::cerr << "Error: Cannot update - no URL recorded for " << name << std::endl;
        return 1;
    }

    std::cout << "Updating " << name << "..." << std::endl;

    // Fetch changes
    int result = utils::exec_command_with_privilege("git", {"-C", record->source_dir, "fetch"});
    if (result != 0) {
        std::cerr << "Error: Failed to fetch updates." << std::endl;
        return 1;
    }

    // Check if there are changes
    std::string status = utils::exec_command("git", {"-C", record->source_dir, "status", "-sb"});

    // Check if ahead of origin
    if (status.find("behind") == std::string::npos) {
        std::cout << name << " is already up to date." << std::endl;
        return 0;
    }

    // Pull changes
    result = utils::exec_command_with_privilege("git", {"-C", record->source_dir, "pull"});
    if (result != 0) {
        std::cerr << "Error: Failed to pull updates." << std::endl;
        return 1;
    }

    // Rebuild
    std::cout << "Rebuilding..." << std::endl;
    result = utils::exec_command_with_privilege("make", {"-C", record->source_dir, "clean"});
    if (result != 0) {
        std::cerr << "Error: Clean failed." << std::endl;
        return 1;
    }
    result = utils::exec_command_with_privilege("make", {"-C", record->source_dir});
    if (result != 0) {
        std::cerr << "Error: Build failed." << std::endl;
        return 1;
    }

    // Reinstall
    std::cout << "Reinstalling..." << std::endl;
    result = utils::exec_command_with_privilege("make", {"-C", record->source_dir, "install"});
    if (result != 0) {
        std::cerr << "Error: Install failed." << std::endl;
        return 1;
    }

    // Update commit hash
    std::string new_hash = utils::exec_command("git", {"-C", record->source_dir, "rev-parse", "HEAD"});
    if (!new_hash.empty() && new_hash.back() == '\n') {
        new_hash.pop_back();
    }

    registry::Record updated_record = *record;
    updated_record.commit_hash = new_hash;
    registry.update(name, updated_record);

    std::cout << "✓ Updated " << name << std::endl;

    return 0;
}

} // namespace commands
} // namespace makekeeper
