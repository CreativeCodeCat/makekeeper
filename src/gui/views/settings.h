#ifndef MAKEKEEPER_GUI_VIEWS_SETTINGS_H
#define MAKEKEEPER_GUI_VIEWS_SETTINGS_H

#include "base.h"

namespace makekeeper {
namespace gui {
namespace views {

class SettingsView : public BaseView {
    Q_OBJECT

public:
    SettingsView(QWidget* parent = nullptr);
    void refresh() override;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_SETTINGS_H
