#ifndef MAKEKEEPER_GUI_VIEWS_REGISTRY_H
#define MAKEKEEPER_GUI_VIEWS_REGISTRY_H

#include "base.h"
#include <QTableWidget>
#include <QTextEdit>

namespace makekeeper {
namespace gui {
namespace views {

class RegistryView : public BaseView {
    Q_OBJECT

public:
    RegistryView(QWidget* parent = nullptr);
    void refresh() override;

private:
    void setup_ui();
    void load_registry();

    QTableWidget* registry_table_;
    QTextEdit* terminal_panel_;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_REGISTRY_H
