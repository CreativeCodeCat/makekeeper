#ifndef MAKEKEEPER_GUI_VIEWS_PACKAGES_H
#define MAKEKEEPER_GUI_VIEWS_PACKAGES_H

#include "base.h"
#include <QTableWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QSet>

namespace makekeeper {
namespace gui {
namespace views {

class PackagesView : public BaseView {
    Q_OBJECT

public:
    PackagesView(QWidget* parent = nullptr);
    void refresh() override;

private:
    void setup_ui();
    void load_packages();
    void on_search_changed(const QString& text);
    void on_package_selected();

    QLineEdit* search_edit_;
    QTableWidget* package_table_;
    QTextEdit* details_panel_;
    QTextEdit* terminal_panel_;
    QSet<QString> installed_names_;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_PACKAGES_H
