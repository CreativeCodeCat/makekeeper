#include "utils.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <git2.h>
#include <curl/curl.h>

namespace fs = std::filesystem;

namespace makekeeper {
namespace utils {

bool is_root() {
    return getuid() == 0;
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

int exec_command_with_privilege(const std::string& command, const std::vector<std::string>& args) {
    std::vector<char*> argv;
    
    if (is_root()) {
        // Already root, execute directly
        argv.push_back(const_cast<char*>(command.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
    } else {
        // Use pkexec for privilege escalation
        argv.push_back(const_cast<char*>("pkexec"));
        argv.push_back(const_cast<char*>(command.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    }

    if (pid == 0) {
        // Child process
        execvp(argv[0], argv.data());
        _exit(127);
    }

    // Parent process
    int status;
    waitpid(pid, &status, 0);

    return WEXITSTATUS(status);
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
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "makekeeper/0.1.0");

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
