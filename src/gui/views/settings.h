#ifndef MAKEKEEPER_GUI_VIEWS_SETTINGS_H
#define MAKEKEEPER_GUI_VIEWS_SETTINGS_H

#include "base.h"
#include <QCheckBox>

namespace makekeeper {
namespace gui {
namespace views {

class SettingsView : public BaseView {
    Q_OBJECT

public:
    SettingsView(QWidget* parent = nullptr);
    void refresh() override;

private:
    void setup_ui();
    void load_settings();
    void save_settings();
    
    QCheckBox* auto_sync_;
    QCheckBox* notifications_;
    QCheckBox* confirm_install_;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_SETTINGS_H
