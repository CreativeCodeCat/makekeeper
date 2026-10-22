#ifndef MAKEKEEPER_CLI_H
#define MAKEKEEPER_CLI_H

#include <string>
#include <vector>
#include <functional>

namespace makekeeper {
namespace cli {

struct Command {
    std::string name;
    std::string description;
    std::function<int(const std::vector<std::string>&)> handler;
};

class CLI {
public:
    static CLI& instance();

    void register_command(const Command& command);
    int run(int argc, char* argv[]);

    void print_help() const;
    void print_version() const;

private:
    CLI() = default;
    std::vector<Command> commands_;
};

// Print an error and return true if args contain an option (anything starting with '-') that the
// command doesn't support, so flags such as --yes are rejected instead of silently ignored
bool reject_unknown_options(const std::vector<std::string>& args, const std::string& usage);

// Print a summary of what is about to happen and ask a yes/no question, defaulting to yes; returns
// false for no or end of input. Without a terminal (the GUI, scripts) it prints only an error
// suggesting --yes and returns false, rather than blocking on or misreading piped input
bool confirm(const std::string& summary, const std::string& question);

} // namespace cli
} // namespace makekeeper

#endif // MAKEKEEPER_CLI_H
