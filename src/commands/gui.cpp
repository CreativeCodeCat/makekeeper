#include "gui.h"
#include "../gui/gui.h"
#include <iostream>

namespace makekeeper {
namespace commands {

int gui(const std::vector<std::string>& args) {
    // Check for help flag
    if (!args.empty() && (args[0] == "--help" || args[0] == "-h")) {
        std::cout << "Usage: makekeeper gui" << std::endl;
        std::cout << std::endl;
        std::cout << "Launch the MakeKeeper graphical user interface." << std::endl;
        return 0;
    }

    return gui::launch(args);
}

} // namespace commands
} // namespace makekeeper
