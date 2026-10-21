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
    if (args.empty()) {
        std::cerr << "Usage: makekeeper update <NAME>" << std::endl;
        return 1;
    }

    std::string name = args[0];

    // Check if root
    if (!utils::is_root()) {
        std::cerr << "Error: This command requires root privileges." << std::endl;
        return 1;
    }

    // Load registry
    auto& registry = registry::Registry::instance();
    if (!registry.load()) {
        std::cerr << "Error: Failed to load registry." << std::endl;
        return 1;
    }

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
    std::string fetch_cmd = "cd " + record->source_dir + " && git fetch";
    int result = system(fetch_cmd.c_str());
    if (result != 0) {
        std::cerr << "Error: Failed to fetch updates." << std::endl;
        return 1;
    }

    // Check if there are changes
    std::string status_cmd = "cd " + record->source_dir + " && git status -sb";
    std::string status = utils::exec_command("git", {"-C", record->source_dir, "status", "-sb"});

    // Check if ahead of origin
    if (status.find("behind") == std::string::npos) {
        std::cout << name << " is already up to date." << std::endl;
        return 0;
    }

    // Pull changes
    std::string pull_cmd = "cd " + record->source_dir + " && git pull";
    result = system(pull_cmd.c_str());
    if (result != 0) {
        std::cerr << "Error: Failed to pull updates." << std::endl;
        return 1;
    }

    // Rebuild
    std::cout << "Rebuilding..." << std::endl;
    std::string build_cmd = "cd " + record->source_dir + " && make clean && make";
    result = system(build_cmd.c_str());
    if (result != 0) {
        std::cerr << "Error: Build failed." << std::endl;
        return 1;
    }

    // Reinstall
    std::cout << "Reinstalling..." << std::endl;
    std::string install_cmd = "cd " + record->source_dir + " && sudo make install";
    result = system(install_cmd.c_str());
    if (result != 0) {
        std::cerr << "Error: Install failed." << std::endl;
        return 1;
    }

    // Update commit hash
    std::string hash_cmd = "cd " + record->source_dir + " && git rev-parse HEAD";
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
