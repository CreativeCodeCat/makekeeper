// makekeeper-helper: the only part of MakeKeeper that runs as root.
//
// makekeeper runs it through pkexec, and the polkit policy in makekeeper.policy.in
// lets polkit remember the authorization for a few minutes, so a whole install or
// uninstall asks for a password once. Every operation checks its arguments, so the
// helper cannot be used to run arbitrary commands or touch arbitrary files.

#include "../config.h"
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;
namespace config = makekeeper::config;

namespace {

int fail(const std::string& message) {
    std::cerr << "makekeeper-helper: " << message << std::endl;
    return 1;
}

int run(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    }
    if (pid == 0) {
        execvp(argv[0], argv.data());
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

// Package names become directory names, so keep them to a safe character set
bool valid_name(const std::string& name) {
    if (name.empty() || name == "." || name == ".." || name.size() > 128) {
        return false;
    }
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '_' && c != '-' && c != '+') {
            return false;
        }
    }
    return true;
}

bool is_plain_absolute(const std::string& path) {
    fs::path p(path);
    return p.is_absolute() && p.lexically_normal() == p;
}

bool ensure_state_dirs() {
    std::error_code ec;
    fs::create_directories(config::CACHE_DIR, ec);
    return !ec;
}

// Atomically replace a file under /var/lib/makekeeper with JSON read from stdin
int write_json(const std::string& target, const char* required_array) {
    std::string input((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());

    json j;
    try {
        j = json::parse(input);
    } catch (const std::exception& e) {
        return fail("input is not valid JSON");
    }
    if (!j.is_object() || !j.contains(required_array) || !j[required_array].is_array()) {
        return fail(std::string("input has no '") + required_array + "' array");
    }

    if (!ensure_state_dirs()) {
        return fail("cannot create " + std::string(config::REGISTRY_DIR));
    }

    std::string temp = target + ".new";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out.is_open()) {
            return fail("cannot write " + temp);
        }
        out << j.dump(2);
        if (!out.good()) {
            return fail("cannot write " + temp);
        }
    }
    chmod(temp.c_str(), 0644);

    std::error_code ec;
    fs::rename(temp, target, ec);
    if (ec) {
        fs::remove(temp, ec);
        return fail("cannot replace " + target);
    }
    return 0;
}

// make -C <dir> [target], for a project directory that contains a Makefile
int op_make(const std::vector<std::string>& args) {
    if (args.empty() || args.size() > 2) {
        return fail("usage: make <DIR> [clean|install|uninstall]");
    }

    const std::string& dir = args[0];
    if (!is_plain_absolute(dir) || !fs::is_directory(dir)) {
        return fail("not a project directory: " + dir);
    }
    if (!fs::exists(dir + "/Makefile") && !fs::exists(dir + "/makefile") && !fs::exists(dir + "/GNUmakefile")) {
        return fail("no Makefile in " + dir);
    }

    std::vector<std::string> command = {"make", "-C", dir};
    if (args.size() == 2) {
        const std::string& target = args[1];
        if (target != "clean" && target != "install" && target != "uninstall") {
            return fail("unsupported make target: " + target);
        }
        command.push_back(target);
    }
    return run(command);
}

// Delete a file or symlink, but only one the registry says MakeKeeper installed
int op_remove(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return fail("usage: remove <PATH>");
    }

    const std::string& path = args[0];
    if (!is_plain_absolute(path)) {
        return fail("refusing to remove non-canonical path: " + path);
    }

    bool tracked = false;
    std::ifstream file(config::REGISTRY_FILE);
    try {
        json j;
        file >> j;
        for (const auto& record : j.at("records")) {
            for (const char* key : {"files", "symlinks"}) {
                for (const auto& entry : record.value(key, json::array())) {
                    if (entry.is_string() && entry.get<std::string>() == path) {
                        tracked = true;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        return fail("cannot read registry");
    }
    if (!tracked) {
        return fail("refusing to remove untracked path: " + path);
    }

    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
        return 0;  // Already gone
    }
    if (S_ISDIR(st.st_mode)) {
        return fail("refusing to remove directory: " + path);
    }
    if (unlink(path.c_str()) != 0) {
        return fail("cannot remove " + path);
    }
    return 0;
}

// Delete a root-owned clone that older versions kept in the cache; sources now live in the user's home
int op_remove_source(const std::vector<std::string>& args) {
    if (args.size() != 1 || !valid_name(args[0])) {
        return fail("usage: remove-source <NAME>");
    }

    std::string target = std::string(config::CACHE_DIR) + "/" + args[0];
    struct stat st;
    if (lstat(target.c_str(), &st) != 0) {
        return 0;  // Already gone
    }
    if (!S_ISDIR(st.st_mode)) {
        return fail("not a source directory: " + target);
    }

    std::error_code ec;
    fs::remove_all(target, ec);
    return ec ? fail("cannot remove " + target) : 0;
}

int op_purge(const std::vector<std::string>& args) {
    if (!args.empty()) {
        return fail("usage: purge");
    }
    std::error_code ec;
    fs::remove_all(config::REGISTRY_DIR, ec);
    return ec ? fail("cannot remove " + std::string(config::REGISTRY_DIR)) : 0;
}

void print_usage() {
    std::cerr << "Usage: makekeeper-helper <OPERATION> [ARGS]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Operations:" << std::endl;
    std::cerr << "  init                       Create the registry and cache directories" << std::endl;
    std::cerr << "  make <DIR> [TARGET]        Run make in a project (TARGET: clean, install, uninstall)" << std::endl;
    std::cerr << "  remove <PATH>              Remove a file tracked in the registry" << std::endl;
    std::cerr << "  remove-source <NAME>       Delete a clone left in the cache by an older version" << std::endl;
    std::cerr << "  write-registry             Replace the registry with JSON from stdin" << std::endl;
    std::cerr << "  write-catalog              Replace the package catalog with JSON from stdin" << std::endl;
    std::cerr << "  purge                      Delete all MakeKeeper state" << std::endl;
    std::cerr << std::endl;
    std::cerr << "This program is run by makekeeper through pkexec; you do not need to run it yourself." << std::endl;
}

} // namespace

int main(int argc, char* argv[]) {
    if (geteuid() != 0) {
        return fail("must be run as root (makekeeper runs it through pkexec)");
    }
    if (argc < 2) {
        print_usage();
        return 2;
    }

    // Files created as root should be readable by everyone, like the rest of /var/lib
    umask(022);

    std::string op = argv[1];
    std::vector<std::string> args(argv + 2, argv + argc);

    if (op == "init") {
        return ensure_state_dirs() ? 0 : fail("cannot create " + std::string(config::CACHE_DIR));
    }
    if (op == "make") {
        return op_make(args);
    }
    if (op == "remove") {
        return op_remove(args);
    }
    if (op == "remove-source") {
        return op_remove_source(args);
    }
    if (op == "write-registry") {
        return write_json(config::REGISTRY_FILE, "records");
    }
    if (op == "write-catalog") {
        return write_json(config::CACHE_FILE, "packages");
    }
    if (op == "purge") {
        return op_purge(args);
    }

    print_usage();
    return 2;
}
