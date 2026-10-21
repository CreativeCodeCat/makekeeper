#include "registry.h"
#include "config.h"
#include "utils.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace makekeeper {
namespace registry {

Registry& Registry::instance() {
    static Registry instance;
    return instance;
}

bool Registry::load() {
    std::ifstream file(config::REGISTRY_FILE);
    if (!file.is_open()) {
        // Registry doesn't exist yet, that's ok
        records_.clear();
        return true;
    }

    try {
        json j;
        file >> j;

        records_.clear();
        for (const auto& item : j["records"]) {
            Record record;
            record.name = item["name"];
            record.url = item["url"];
            record.source_dir = item["source_dir"];
            record.commit_hash = item["commit_hash"];
            record.install_date = item["install_date"];
            record.files = item["files"].get<std::vector<std::string>>();
            record.symlinks = item["symlinks"].get<std::vector<std::string>>();
            records_.push_back(record);
        }
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool Registry::save() {
    // Save to temp file first
    std::string temp_file = "/tmp/registry.json.tmp";
    std::ofstream file(temp_file);
    if (!file.is_open()) {
        return false;
    }

    try {
        json j;
        j["records"] = json::array();

        for (const auto& record : records_) {
            json item;
            item["name"] = record.name;
            item["url"] = record.url;
            item["source_dir"] = record.source_dir;
            item["commit_hash"] = record.commit_hash;
            item["install_date"] = record.install_date;
            item["files"] = record.files;
            item["symlinks"] = record.symlinks;
            j["records"].push_back(item);
        }

        file << j.dump(2);
        file.close();

        // Move to final location with privilege if not root
        if (!makekeeper::utils::is_root()) {
            // Ensure registry directory exists
            makekeeper::utils::exec_command_with_privilege("mkdir", {"-p", config::REGISTRY_DIR});
            
            // Move file with pkexec
            int result = makekeeper::utils::exec_command_with_privilege("mv", {temp_file, config::REGISTRY_FILE});
            return result == 0;
        } else {
            try {
                fs::rename(temp_file, config::REGISTRY_FILE);
            } catch (const fs::filesystem_error& e) {
                // Cross-device link error, use copy + remove
                fs::copy_file(temp_file, config::REGISTRY_FILE, fs::copy_options::overwrite_existing);
                fs::remove(temp_file);
            }
            return true;
        }
    } catch (const std::exception& e) {
        return false;
    }
}

std::optional<Record> Registry::get(const std::string& name) const {
    for (const auto& record : records_) {
        if (record.name == name) {
            return record;
        }
    }
    return std::nullopt;
}

std::vector<Record> Registry::list() const {
    return records_;
}

bool Registry::add(const Record& record) {
    if (exists(record.name)) {
        return false;
    }
    records_.push_back(record);
    return save();
}

bool Registry::remove(const std::string& name) {
    auto it = std::remove_if(records_.begin(), records_.end(),
        [&name](const Record& r) { return r.name == name; });
    if (it == records_.end()) {
        return false;
    }
    records_.erase(it, records_.end());
    return save();
}

bool Registry::update(const std::string& name, const Record& record) {
    for (auto& r : records_) {
        if (r.name == name) {
            r = record;
            return save();
        }
    }
    return false;
}

bool Registry::exists(const std::string& name) const {
    return get(name).has_value();
}

} // namespace registry
} // namespace makekeeper
