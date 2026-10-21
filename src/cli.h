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

} // namespace cli
} // namespace makekeeper

#endif // MAKEKEEPER_CLI_H
