#include "gui.h"
#include "../gui/gui.h"
#include "../utils.h"
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

    if (utils::is_root()) {
        std::cerr << "Warning: The GUI does not need to run as root. MakeKeeper asks for" << std::endl;
        std::cerr << "         permission with pkexec when an action needs it." << std::endl;
        std::cerr << "         Run 'makekeeper gui' as your normal user instead." << std::endl;
        std::cerr << "         Using settings from: " << utils::user_config_dir() << std::endl;
    }

    return gui::launch(args);
}

} // namespace commands
} // namespace makekeeper
