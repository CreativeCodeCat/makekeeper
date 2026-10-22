#ifndef MAKEKEEPER_GUI_VIEWS_PACKAGES_H
#define MAKEKEEPER_GUI_VIEWS_PACKAGES_H

#include "base.h"
#include <QTableWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QProcess>
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
    QWidget* create_details_panel();
    void load_packages();
    void on_search_changed(const QString& text);
    void on_package_selected();
    void show_details(int row);
    void on_install_clicked();
    void run_command(const QStringList& args);
    void log_to_terminal(const QString& message);

    QLineEdit* search_edit_;
    QTableWidget* package_table_;
    QTextEdit* terminal_panel_;
    QSet<QString> installed_names_;

    // Details panel: an empty page until a package is selected, then the details page
    QStackedWidget* details_stack_;
    QLabel* name_label_;
    QLabel* meta_label_;
    QLabel* status_label_;
    QLabel* description_label_;
    QLabel* repo_label_;
    QLabel* installed_title_;
    QLabel* installed_label_;
    QPushButton* install_btn_;
    QPushButton* repo_btn_;

    QString selected_name_;
    QString selected_url_;
    QProcess* process_;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_PACKAGES_H
