#ifndef MAKEKEEPER_GUI_WINDOW_H
#define MAKEKEEPER_GUI_WINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include "views/base.h"
#include "views/overview.h"
#include "views/packages.h"
#include "views/updates.h"
#include "views/registry.h"
#include "views/settings.h"

namespace makekeeper {
namespace gui {

class MakekeeperWindow : public QMainWindow {
public:
    MakekeeperWindow(QWidget* parent = nullptr);
    ~MakekeeperWindow();

private:
    void setup_ui();
    void on_nav_clicked(int index);

    QStackedWidget* content_stack_;
    QPushButton* nav_overview_;
    QPushButton* nav_packages_;
    QPushButton* nav_updates_;
    QPushButton* nav_registry_;
    QPushButton* nav_settings_;
};

} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_WINDOW_H
