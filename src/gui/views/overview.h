#ifndef MAKEKEEPER_GUI_VIEWS_OVERVIEW_H
#define MAKEKEEPER_GUI_VIEWS_OVERVIEW_H

#include "base.h"
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProcess>

namespace makekeeper {
namespace gui {
namespace views {

class OverviewView : public BaseView {
    Q_OBJECT

public:
    OverviewView(QWidget* parent = nullptr);
    void refresh() override;

private:
    void setup_ui();
    void update_stats();
    void log_to_terminal(const QString& message);
    void run_command(const QString& command);

    QLabel* total_installed_label_;
    QLabel* git_repos_label_;
    QLabel* sync_status_label_;
    QTextEdit* terminal_panel_;
    QProcess* process_;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_OVERVIEW_H
