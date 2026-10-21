#include "gui.h"
#include "window.h"
#include <QApplication>

namespace makekeeper {
namespace gui {

int launch(const std::vector<std::string>& args) {
    int argc = 0;
    char** argv = nullptr;

    QApplication app(argc, argv);

    MakekeeperWindow window;
    window.show();

    return app.exec();
}

} // namespace gui
} // namespace makekeeper
