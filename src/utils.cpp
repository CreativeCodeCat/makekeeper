#include "utils.h"
#include "config.h"
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <git2.h>
#include <curl/curl.h>

namespace fs = std::filesystem;

namespace makekeeper {
namespace utils {

bool is_root() {
    return getuid() == 0;
}

// The real user behind sudo or pkexec, or nullptr when not running as root on someone's behalf
static const passwd* invoking_user() {
    if (!is_root()) {
        return nullptr;
    }

    const char* sudo_user = std::getenv("SUDO_USER");
    if (sudo_user && *sudo_user && std::strcmp(sudo_user, "root") != 0) {
        return getpwnam(sudo_user);
    }

    const char* pkexec_uid = std::getenv("PKEXEC_UID");
    if (pkexec_uid && *pkexec_uid) {
        char* end = nullptr;
        unsigned long uid = std::strtoul(pkexec_uid, &end, 10);
        if (*end == '\0' && uid != 0) {
            return getpwuid(static_cast<uid_t>(uid));
        }
    }

    return nullptr;
}

// The real user's home directory, even under sudo or pkexec (which reset HOME)
static std::string user_home() {
    if (const passwd* pw = invoking_user()) {
        return pw->pw_dir;
    }

    const char* home = std::getenv("HOME");
    if (home && *home) {
        return home;
    }
    const passwd* pw = getpwuid(getuid());
    return pw ? pw->pw_dir : "/tmp";
}

// An XDG base directory such as XDG_CONFIG_HOME, falling back to a path under the home directory.
// Under sudo or pkexec the variable belongs to root (or is cleared), so always use the fallback
static std::string xdg_dir(const char* variable, const std::string& fallback) {
    const char* value = std::getenv(variable);
    if (!invoking_user() && value && value[0] == '/') {
        return value;
    }
    return user_home() + "/" + fallback;
}

std::string user_config_dir() {
    return xdg_dir("XDG_CONFIG_HOME", ".config") + "/makekeeper";
}

std::string user_sources_dir() {
    return xdg_dir("XDG_DATA_HOME", ".local/share") + "/makekeeper/sources";
}

bool is_legacy_source(const std::string& dir) {
    return fs::path(dir).lexically_normal().parent_path() == fs::path(config::CACHE_DIR);
}

void chown_to_invoking_user(const std::string& path) {
    if (const passwd* pw = invoking_user()) {
        // Best effort: if this fails the file still works, it is just owned by root
        if (chown(path.c_str(), pw->pw_uid, pw->pw_gid) != 0) {
            return;
        }
    }
}

std::string exec_command(const std::string& command) {
    return exec_command(command, {});
}

std::string exec_command(const std::string& command, const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(command.c_str()));
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return "";
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return "";
    }

    if (pid == 0) {
        // Child process
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(command.c_str(), argv.data());
        _exit(127);
    }

    // Parent process
    close(pipefd[1]);

    std::string result;
    char buffer[4096];
    ssize_t count;
    while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        result.append(buffer, count);
    }

    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);

    return result;
}

int exec_command_exit_code(const std::string& command, const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(command.c_str()));
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    }

    if (pid == 0) {
        // Child process
        execvp(command.c_str(), argv.data());
        _exit(127);
    }

    // Parent process
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return -1;
    }
}

int exec_as_user(const std::string& command, const std::vector<std::string>& args) {
    const passwd* pw = invoking_user();
    if (!pw) {
        return exec_command_exit_code(command, args);
    }

    // Copy what the child needs before forking; getpwnam's buffer is static
    uid_t uid = pw->pw_uid;
    gid_t gid = pw->pw_gid;
    std::string user = pw->pw_name;
    std::string home = pw->pw_dir;

    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(command.c_str()));
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    }

    if (pid == 0) {
        // Child process: drop root for good before running anything
        if (initgroups(user.c_str(), gid) != 0 || setgid(gid) != 0 || setuid(uid) != 0) {
            _exit(126);
        }
        setenv("HOME", home.c_str(), 1);
        setenv("USER", user.c_str(), 1);
        setenv("LOGNAME", user.c_str(), 1);
        execvp(command.c_str(), argv.data());
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// Find makekeeper-helper: installed in <prefix>/lib/makekeeper, or next to the binary in a build tree
static std::string find_helper() {
    std::error_code ec;
    fs::path exe_dir = fs::read_symlink("/proc/self/exe", ec).parent_path();
    if (ec) {
        return "";
    }

    for (const fs::path& candidate : {
             (exe_dir / "../lib/makekeeper/makekeeper-helper").lexically_normal(),
             exe_dir / "makekeeper-helper",
         }) {
        if (fs::exists(candidate)) {
            return candidate.string();
        }
    }
    return "";
}

// Root cannot read inside a user's AppImage mount, so copy the helper somewhere it can
static std::string copy_helper_out_of_appimage(const std::string& helper) {
    const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    std::string dir;
    if (runtime_dir && runtime_dir[0] == '/') {
        dir = std::string(runtime_dir) + "/makekeeper";
        mkdir(dir.c_str(), 0700);
    } else {
        char tmpl[] = "/tmp/makekeeper-helper-XXXXXX";
        if (!mkdtemp(tmpl)) {
            return "";
        }
        dir = tmpl;
    }

    std::string target = dir + "/makekeeper-helper";
    std::error_code ec;
    fs::copy_file(helper, target, fs::copy_options::overwrite_existing, ec);
    if (ec || chmod(target.c_str(), 0700) != 0) {
        return "";
    }
    return target;
}

bool can_get_root() {
    // With no_new_privs set, the kernel ignores pkexec's setuid bit and it fails with a cryptic
    // "pkexec must be setuid root". Sandboxes and some IDE or assistant terminals set it, and every
    // process started from them inherits it, so explain instead
    if (!is_root() && prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) == 1) {
        std::cerr << "Error: Can't get root privileges from here. MakeKeeper was started from a sandboxed" << std::endl;
        std::cerr << "       environment (no_new_privs is set), which stops pkexec from working." << std::endl;
        std::cerr << "       Run MakeKeeper from a normal terminal or your application menu instead." << std::endl;
        return false;
    }
    return true;
}

int run_helper(const std::vector<std::string>& args, const std::string& input) {
    static std::string helper = [] {
        std::string path = find_helper();
        if (!path.empty() && std::getenv("APPIMAGE") && !is_root()) {
            path = copy_helper_out_of_appimage(path);
        }
        return path;
    }();

    if (helper.empty()) {
        std::cerr << "Error: makekeeper-helper not found. Reinstall MakeKeeper." << std::endl;
        return 127;
    }

    if (!can_get_root()) {
        return 126;
    }

    std::vector<std::string> command;
    if (!is_root()) {
        command.push_back("pkexec");
    }
    command.push_back(helper);
    command.insert(command.end(), args.begin(), args.end());

    std::vector<char*> argv;
    for (const auto& arg : command) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        // Child process: read input from the pipe
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(argv[0], argv.data());
        _exit(127);
    }

    // Parent process: send input, then close so the helper sees EOF.
    // Ignore SIGPIPE in case the helper exits first (for example, authentication was cancelled)
    close(pipefd[0]);
    auto previous_sigpipe = signal(SIGPIPE, SIG_IGN);
    size_t written = 0;
    while (written < input.size()) {
        ssize_t n = write(pipefd[1], input.data() + written, input.size() - written);
        if (n <= 0) {
            break;
        }
        written += static_cast<size_t>(n);
    }
    close(pipefd[1]);
    signal(SIGPIPE, previous_sigpipe);

    int status;
    waitpid(pid, &status, 0);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

bool clone_source(const std::string& url, const std::string& dir) {
    if (exec_as_user("mkdir", {"-p", fs::path(dir).parent_path().string()}) != 0) {
        return false;
    }

    std::error_code ec;
    fs::remove_all(dir, ec);
    if (ec) {
        return false;
    }
    return exec_as_user("git", {"clone", url, dir}) == 0;
}

int git_update_source(const std::string& dir, const std::string& action) {
    return exec_as_user("git", {"-C", dir, action});
}

std::string git_query(const std::string& dir, const std::vector<std::string>& args) {
    // Under sudo the clone belongs to the real user, not root; allow reading it anyway
    std::vector<std::string> command = {"-c", "safe.directory=" + dir, "--no-optional-locks", "-C", dir};
    command.insert(command.end(), args.begin(), args.end());

    std::string output = exec_command("git", command);
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }
    return output;
}

std::string get_timestamp() {
    time_t now = time(nullptr);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

std::string get_latest_tag(const std::string& url) {
    git_libgit2_init();

    git_repository* repo = nullptr;
    git_remote* remote = nullptr;
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;

    std::string result = "latest";

    if (git_remote_create_anonymous(&remote, nullptr, url.c_str()) != 0) {
        git_libgit2_shutdown();
        return result;
    }

    git_remote_connect(remote, GIT_DIRECTION_FETCH, nullptr, nullptr, nullptr);
    git_remote_download(remote, nullptr, nullptr);
    git_remote_disconnect(remote);

    // Get tags using ls-remote
    const git_remote_head** heads;
    size_t heads_len;
    if (git_remote_ls(&heads, &heads_len, remote) == 0) {
        std::string last_tag;
        for (size_t i = 0; i < heads_len; i++) {
            std::string tag = heads[i]->name;
            if (tag.find("refs/tags/") == 0) {
                std::string tag_name = tag.substr(10);
                // Ignore annotated tag dereferences (^{})
                if (tag_name.find("^{}") == std::string::npos) {
                    last_tag = tag_name;
                }
            }
        }

        if (!last_tag.empty()) {
            // Remove 'v' prefix if present
            if (last_tag[0] == 'v') {
                result = last_tag.substr(1);
            } else {
                result = last_tag;
            }
        }
    }

    git_remote_free(remote);
    git_libgit2_shutdown();

    return result;
}

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

bool download_file(const std::string& url, const std::string& output_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    FILE* fp = fopen(output_path.c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, (std::string("makekeeper/") + config::VERSION).c_str());

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}

bool ensure_directory(const std::string& path) {
    try {
        return fs::create_directories(path);
    } catch (const fs::filesystem_error&) {
        return fs::exists(path);
    }
}

bool remove_directory(const std::string& path) {
    try {
        return fs::remove_all(path) > 0;
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

bool copy_file(const std::string& src, const std::string& dst) {
    try {
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
        return true;
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

bool create_symlink(const std::string& target, const std::string& link) {
    return symlink(target.c_str(), link.c_str()) == 0;
}

bool remove_symlink(const std::string& link) {
    return unlink(link.c_str()) == 0;
}

bool is_symlink(const std::string& path) {
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISLNK(st.st_mode);
}

std::string resolve_symlink(const std::string& path) {
    char buf[PATH_MAX];
    ssize_t len = readlink(path.c_str(), buf, sizeof(buf) - 1);
    if (len == -1) {
        return path;
    }
    buf[len] = '\0';
    return std::string(buf);
}

} // namespace utils
} // namespace makekeeper
