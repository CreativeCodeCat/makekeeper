#ifndef MAKEKEEPER_UTILS_H
#define MAKEKEEPER_UTILS_H

#include <string>
#include <vector>
#include <optional>

namespace makekeeper {
namespace utils {

// Check if running as root
bool is_root();

// Get the makekeeper config directory of the user who ran the command (follows sudo and pkexec)
std::string user_config_dir();

// Get the directory holding the user's package source clones (follows sudo and pkexec)
std::string user_sources_dir();

// Whether a source directory is a root-owned clone in /var/lib/makekeeper/cache from an older version
bool is_legacy_source(const std::string& dir);

// Give a path to the user who ran the command when running as root via sudo or pkexec
void chown_to_invoking_user(const std::string& path);

// Execute command and return output
std::string exec_command(const std::string& command);

// Execute command with args
std::string exec_command(const std::string& command, const std::vector<std::string>& args);

// Execute command and return exit code
int exec_command_exit_code(const std::string& command, const std::vector<std::string>& args = {});

// Execute command as the real user, dropping root when running under sudo or pkexec
int exec_as_user(const std::string& command, const std::vector<std::string>& args = {});

// Whether this process can become root through pkexec. When it can't (no_new_privs is set, as in
// sandboxes and some IDE or assistant terminals), prints why and returns false, so commands can
// stop before changing anything
bool can_get_root();

// Run a makekeeper-helper operation as root, asking for permission through pkexec when needed
int run_helper(const std::vector<std::string>& args, const std::string& input = "");

// Clone a repository as the real user, replacing anything already at dir
bool clone_source(const std::string& url, const std::string& dir);

// "make" if dir has a Makefile, "cmake" if it has a CMakeLists.txt, "" if it has neither.
// Checked in that order, so a CMake project with a thin Makefile wrapper keeps using make.
std::string detect_build_system(const std::string& dir);

// Run git fetch or pull in a package's source directory as the real user
int git_update_source(const std::string& dir, const std::string& action);

// Run a read-only git command in a package's source directory and return its output
std::string git_query(const std::string& dir, const std::vector<std::string>& args);

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
