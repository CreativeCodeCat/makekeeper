#ifndef MAKEKEEPER_GUI_VIEWS_UPDATES_H
#define MAKEKEEPER_GUI_VIEWS_UPDATES_H

#include "base.h"
#include <QDateTime>
#include <QHash>
#include <QLabel>
#include <QProcess>
#include <QSet>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <vector>

namespace makekeeper {
namespace gui {
namespace views {

class UpdatesView : public BaseView {
    Q_OBJECT

public:
    UpdatesView(QWidget* parent = nullptr);
    void refresh() override;

private:
    enum class Status { NotChecked, Checking, UpToDate, UpdateAvailable, Unknown, Failed };

    // A package installed from Git, and what the last check found for it
    struct Package {
        QString name;
        QString url;
        QString installed_commit;
        QString latest_commit;
        QString error;
        Status status = Status::NotChecked;
    };

    void setup_ui();
    void load_packages();
    bool needs_check() const;
    void check_for_updates();
    void finish_check(const QString& url, const QString& latest_commit, const QString& error);
    Status status_for(const Package& package) const;
    void render();
    void update_summary();
    void run_update(const QString& name);
    void log_to_terminal(const QString& message);

    QLabel* summary_label_;
    QPushButton* check_btn_;
    QPushButton* update_all_btn_;
    QStackedWidget* content_stack_;
    QTableWidget* table_;
    QTextEdit* terminal_panel_;

    std::vector<Package> packages_;

    // Latest commit (or error) per repository URL from the last check, so revisiting
    // the page doesn't hit the network every time
    QHash<QString, QString> latest_by_url_;
    QHash<QString, QString> error_by_url_;
    QDateTime last_checked_;
    QSet<QString> checking_urls_;  // repositories whose check is still running
    int pending_checks_;
    int check_generation_;

    QProcess* update_process_;
    QString updating_name_;  // package being updated, or "*" for all
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_UPDATES_H
