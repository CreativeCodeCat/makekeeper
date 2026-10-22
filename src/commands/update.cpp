#include "update.h"
#include "../registry.h"
#include "../utils.h"
#include "../config.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace makekeeper {
namespace commands {

enum class UpdateResult { Updated, UpToDate, Failed };

// Older versions kept clones in /var/lib/makekeeper/cache, owned by root. Clone the package
// again into the user's sources directory, delete the old copy and point the record at the new one.
static bool migrate_legacy_source(registry::Record& record, registry::Registry& registry, const std::string& indent) {
    if (!utils::is_legacy_source(record.source_dir)) {
        return true;
    }

    std::string old_dir = record.source_dir;
    std::string new_dir = utils::user_sources_dir() + "/" + record.name;
    std::cout << indent << "Moving source from " << old_dir << " to " << new_dir << "..." << std::endl;

    if (!utils::clone_source(record.url, new_dir)) {
        std::cerr << indent << "Error: Failed to clone repository." << std::endl;
        return false;
    }
    if (utils::run_helper({"remove-source", fs::path(old_dir).filename().string()}) != 0) {
        std::cerr << indent << "Warning: Failed to remove " << old_dir << std::endl;
    }

    record.source_dir = new_dir;
    registry.update(record.name, record);
    return true;
}

static UpdateResult update_package(registry::Record record, registry::Registry& registry, const std::string& indent) {
    if (!migrate_legacy_source(record, registry, indent)) {
        return UpdateResult::Failed;
    }

    // Fetch changes
    if (utils::git_update_source(record.source_dir, "fetch") != 0) {
        std::cerr << indent << "Error: Failed to fetch updates." << std::endl;
        return UpdateResult::Failed;
    }

    // Up to date when nothing new is upstream and the installed commit is the checked-out one
    // (a freshly migrated clone is already at the latest commit, but the installed build may not be)
    std::string status = utils::git_query(record.source_dir, {"status", "-sb"});
    std::string head = utils::git_query(record.source_dir, {"rev-parse", "HEAD"});
    bool behind = status.find("behind") != std::string::npos;
    bool installed_is_head = record.commit_hash.empty() || record.commit_hash == head;
    if (!behind && installed_is_head) {
        return UpdateResult::UpToDate;
    }

    // Pull changes
    if (behind && utils::git_update_source(record.source_dir, "pull") != 0) {
        std::cerr << indent << "Error: Failed to pull updates." << std::endl;
        return UpdateResult::Failed;
    }

    // Rebuild as the user
    std::cout << indent << "Rebuilding..." << std::endl;
    if (utils::exec_as_user("make", {"-C", record.source_dir, "clean"}) != 0) {
        std::cerr << indent << "Error: Clean failed." << std::endl;
        return UpdateResult::Failed;
    }
    if (utils::exec_as_user("make", {"-C", record.source_dir}) != 0) {
        std::cerr << indent << "Error: Build failed." << std::endl;
        return UpdateResult::Failed;
    }

    // Reinstall with privilege
    std::cout << indent << "Reinstalling..." << std::endl;
    if (utils::run_helper({"make", record.source_dir, "install"}) != 0) {
        std::cerr << indent << "Error: Install failed." << std::endl;
        return UpdateResult::Failed;
    }

    // Update commit hash
    record.commit_hash = utils::git_query(record.source_dir, {"rev-parse", "HEAD"});
    registry.update(record.name, record);

    return UpdateResult::Updated;
}

int update(const std::vector<std::string>& args) {
    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << "Usage: makekeeper update [NAME]" << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  NAME    Package name to update (optional, updates all if not specified)" << std::endl;
        return 0;
    }

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

            switch (update_package(record, registry, "  ")) {
                case UpdateResult::Updated:
                    std::cout << "  ✓ Updated " << record.name << std::endl;
                    updated_count++;
                    break;
                case UpdateResult::UpToDate:
                    std::cout << "  Already up to date." << std::endl;
                    skipped_count++;
                    break;
                case UpdateResult::Failed:
                    break;
            }
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

    switch (update_package(*record, registry, "")) {
        case UpdateResult::Updated:
            std::cout << "✓ Updated " << name << std::endl;
            return 0;
        case UpdateResult::UpToDate:
            std::cout << name << " is already up to date." << std::endl;
            return 0;
        case UpdateResult::Failed:
            return 1;
    }

    return 1;
}

} // namespace commands
} // namespace makekeeper
