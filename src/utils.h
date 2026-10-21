#ifndef MAKEKEEPER_UTILS_H
#define MAKEKEEPER_UTILS_H

#include <string>
#include <vector>
#include <optional>

namespace makekeeper {
namespace utils {

// Check if running as root
bool is_root();

// Execute command and return output
std::string exec_command(const std::string& command);

// Execute command with args
std::string exec_command(const std::string& command, const std::vector<std::string>& args);

// Execute command with pkexec if not root
int exec_command_with_privilege(const std::string& command, const std::vector<std::string>& args = {});

// Get current timestamp
std::string get_timestamp();

// Get latest git tag from repository
std::string get_latest_tag(const std::string& url);

// Download file from URL
bool download_file(const std::string& url, const std::string& output_path);

// Create directory if it doesn't exist
bool ensure_directory(const std::string& path);

// Remove directory recursively
bool remove_directory(const std::string& path);

// Copy file
bool copy_file(const std::string& src, const std::string& dst);

// Create symlink
bool create_symlink(const std::string& target, const std::string& link);

// Remove symlink
bool remove_symlink(const std::string& link);

// Check if path is a symlink
bool is_symlink(const std::string& path);

// Resolve symlink
std::string resolve_symlink(const std::string& path);

} // namespace utils
} // namespace makekeeper

#endif // MAKEKEEPER_UTILS_H
