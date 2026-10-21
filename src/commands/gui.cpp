#include "gui.h"
#include "../gui/gui.h"

namespace makekeeper {
namespace commands {

int gui(const std::vector<std::string>& args) {
    return gui::launch(args);
}

} // namespace commands
} // namespace makekeeper
