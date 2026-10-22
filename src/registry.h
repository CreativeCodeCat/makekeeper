#ifndef MAKEKEEPER_REGISTRY_H
#define MAKEKEEPER_REGISTRY_H

#include <string>
#include <vector>
#include <optional>

namespace makekeeper {
namespace registry {

struct Record {
    std::string name;
    std::string url;
    std::string source_dir;
    std::string commit_hash;
    std::string install_date;
    std::vector<std::string> files;
    std::vector<std::string> symlinks;
    std::string build_system = "make";  // "make" or "cmake"
};

class Registry {
public:
    static Registry& instance();

    bool load();
    bool save();

    std::optional<Record> get(const std::string& name) const;
    std::vector<Record> list() const;
    bool add(const Record& record);
    bool remove(const std::string& name);
    bool update(const std::string& name, const Record& record);

    bool exists(const std::string& name) const;

private:
    Registry() = default;
    std::vector<Record> records_;
};

} // namespace registry
} // namespace makekeeper

#endif // MAKEKEEPER_REGISTRY_H
