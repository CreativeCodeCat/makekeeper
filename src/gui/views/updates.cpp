#include "updates.h"
#include "../theme.h"
#include "../../registry.h"
#include <QAbstractItemView>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QProcessEnvironment>
#include <QTimer>
#include <QVBoxLayout>

namespace makekeeper {
namespace gui {
namespace views {

namespace {

// Check again when the page is opened if the last check is older than this
constexpr qint64 CHECK_INTERVAL_SECONDS = 10 * 60;

// Give up on a repository that doesn't answer, for example a stalled connection
constexpr int CHECK_TIMEOUT_MS = 30 * 1000;

enum Column { NameColumn, InstalledColumn, LatestColumn, StatusColumn, ActionColumn, ColumnCount };

// A smaller version of the primary button, sized to fit in a table row
QString row_button_style() {
    return "QPushButton { background-color: palette(highlight); color: palette(highlighted-text); border: 1px solid palette(highlight); border-radius: 4px; padding: 3px 14px; font-size: 12px; font-weight: 500; }"
           "QPushButton:hover { border-color: palette(highlighted-text); }"
           "QPushButton:disabled { background-color: palette(button); color: palette(mid); border-color: palette(mid); }";
}

} // namespace

UpdatesView::UpdatesView(QWidget* parent)
    : BaseView(parent), pending_checks_(0), check_generation_(0), update_process_(nullptr) {
    setup_ui();
    // Don't touch the network at startup; the first check runs when the page is opened
    load_packages();
    render();
}

void UpdatesView::setup_ui() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // Title
    QLabel* title = new QLabel("Updates", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(window-text);");
    layout->addWidget(title);

    // Summary and actions
    QHBoxLayout* header_layout = new QHBoxLayout();
    header_layout->setSpacing(10);

    summary_label_ = new QLabel(this);
    summary_label_->setStyleSheet("font-size: 13px;");
    theme::set_secondary(summary_label_);
    header_layout->addWidget(summary_label_, 1);

    check_btn_ = new QPushButton("Check for Updates", this);
    check_btn_->setToolTip("Ask each package's Git repository for its latest commit");
    check_btn_->setStyleSheet(theme::secondary_button_style());
    QObject::connect(check_btn_, &QPushButton::clicked, this, [this]() { check_for_updates(); });
    header_layout->addWidget(check_btn_);

    update_all_btn_ = new QPushButton("Update All", this);
    update_all_btn_->setToolTip("Pull, rebuild and reinstall every package installed from Git");
    update_all_btn_->setStyleSheet(theme::primary_button_style());
    QObject::connect(update_all_btn_, &QPushButton::clicked, this, [this]() { run_update("*"); });
    header_layout->addWidget(update_all_btn_);

    layout->addLayout(header_layout);

    // Package table, or a message when nothing is installed from Git
    content_stack_ = new QStackedWidget(this);

    table_ = new QTableWidget(content_stack_);
    table_->setColumnCount(ColumnCount);
    table_->setHorizontalHeaderLabels({"Package", "Installed", "Latest", "Status", ""});
    table_->horizontalHeader()->setSectionResizeMode(NameColumn, QHeaderView::Stretch);
    for (int column : {InstalledColumn, LatestColumn, StatusColumn, ActionColumn}) {
        table_->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Fixed);
    }
    table_->setColumnWidth(InstalledColumn, 110);
    table_->setColumnWidth(LatestColumn, 110);
    table_->setColumnWidth(StatusColumn, 170);
    table_->setColumnWidth(ActionColumn, 110);
    table_->verticalHeader()->setVisible(false);
    table_->verticalHeader()->setDefaultSectionSize(38);
    table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table_->setShowGrid(true);
    table_->setStyleSheet(
        "QTableWidget { background-color: palette(base); color: palette(text); border: 1px solid palette(mid); border-radius: 4px; }"
        "QTableWidget::item { padding: 4px; border: none; }"
        "QTableWidget::item:selected { background-color: palette(highlight); color: palette(highlighted-text); }"
        "QHeaderView::section { background-color: palette(window); color: palette(window-text); padding: 6px 8px; border: none; border-bottom: 1px solid palette(mid); font-weight: bold; font-size: 11px; }"
    );
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    theme::remove_focus_frame(table_);
    content_stack_->addWidget(table_);

    QLabel* empty_label = new QLabel(
        "No packages installed from Git yet.\nInstall one from the Packages page, and its updates will show up here.",
        content_stack_);
    empty_label->setAlignment(Qt::AlignCenter);
    empty_label->setStyleSheet("font-size: 13px;");
    theme::set_secondary(empty_label);
    content_stack_->addWidget(empty_label);

    layout->addWidget(content_stack_, 1);

    // Terminal logs
    QFrame* terminal_frame = new QFrame(this);
    terminal_frame->setMaximumHeight(140);
    terminal_frame->setStyleSheet("QFrame { background-color: palette(base); border: 1px solid palette(mid); border-radius: 4px; }");

    QVBoxLayout* terminal_frame_layout = new QVBoxLayout(terminal_frame);
    terminal_frame_layout->setContentsMargins(0, 0, 0, 0);

    terminal_panel_ = new QTextEdit(this);
    terminal_panel_->setReadOnly(true);
    terminal_panel_->setFrameStyle(QFrame::NoFrame);
    terminal_panel_->setStyleSheet(
        "QTextEdit { background-color: palette(base); color: palette(text); font-family: 'Consolas', 'Monospace'; font-size: 11px; border: none; padding: 10px; outline: none; }"
        "QTextEdit:hover { border: none; }"
        "QTextEdit:focus { border: none; }"
    );
    terminal_panel_->setText("Terminal Logs\n---------\n> MakeKeeper Terminal Ready\n");

    terminal_frame_layout->addWidget(terminal_panel_);
    layout->addWidget(terminal_frame);
}

void UpdatesView::refresh() {
    if (update_process_) {
        return;
    }
    load_packages();
    if (needs_check()) {
        check_for_updates();
    } else {
        render();
    }
}

void UpdatesView::load_packages() {
    auto& registry = registry::Registry::instance();
    registry.load();

    packages_.clear();
    for (const auto& record : registry.list()) {
        // Only packages installed from Git can be checked for updates
        if (record.name.empty() || record.url.empty()) {
            continue;
        }

        Package package;
        package.name = QString::fromStdString(record.name);
        package.url = QString::fromStdString(record.url);
        package.installed_commit = QString::fromStdString(record.commit_hash);
        package.latest_commit = latest_by_url_.value(package.url);
        package.error = error_by_url_.value(package.url);
        package.status = status_for(package);
        packages_.push_back(package);
    }
}

bool UpdatesView::needs_check() const {
    if (packages_.empty() || pending_checks_ > 0) {
        return false;
    }
    if (!last_checked_.isValid() || last_checked_.secsTo(QDateTime::currentDateTime()) > CHECK_INTERVAL_SECONDS) {
        return true;
    }
    // A package installed since the last check hasn't been looked up yet
    for (const auto& package : packages_) {
        if (!latest_by_url_.contains(package.url) && !error_by_url_.contains(package.url)) {
            return true;
        }
    }
    return false;
}

void UpdatesView::check_for_updates() {
    if (update_process_ || packages_.empty()) {
        return;
    }

    // Results from an earlier check that is still running are ignored
    check_generation_++;
    int generation = check_generation_;
    pending_checks_ = 0;
    checking_urls_.clear();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("GIT_TERMINAL_PROMPT", "0");  // fail instead of waiting for a password

    for (auto& package : packages_) {
        package.status = Status::Checking;
        if (checking_urls_.contains(package.url)) {
            continue;  // two packages from the same repository need one check
        }
        checking_urls_.insert(package.url);
        pending_checks_++;

        QProcess* git = new QProcess(this);
        git->setProcessEnvironment(env);
        git->setStandardInputFile(QProcess::nullDevice());
        QString url = package.url;

        QObject::connect(git, &QProcess::finished, this, [this, git, url, generation](int exit_code, QProcess::ExitStatus exit_status) {
            // "git ls-remote <url> HEAD" prints "<commit>\tHEAD"
            QString commit = QString::fromUtf8(git->readAllStandardOutput()).section('\t', 0, 0).trimmed();
            QString error = QString::fromUtf8(git->readAllStandardError()).trimmed();
            git->deleteLater();
            if (generation != check_generation_) {
                return;
            }
            bool ok = exit_status == QProcess::NormalExit && exit_code == 0 && commit.size() == 40;
            if (exit_status == QProcess::CrashExit) {
                error = "The repository didn't answer in time.";
            }
            finish_check(url, ok ? commit : QString(), ok ? QString() : (error.isEmpty() ? "Couldn't reach the repository." : error));
        });

        QObject::connect(git, &QProcess::errorOccurred, this, [this, git, url, generation](QProcess::ProcessError error) {
            if (error != QProcess::FailedToStart) {
                return;
            }
            git->deleteLater();
            if (generation == check_generation_) {
                finish_check(url, QString(), "git is not installed.");
            }
        });

        QTimer::singleShot(CHECK_TIMEOUT_MS, git, [git]() { git->kill(); });
        git->start("git", {"ls-remote", url, "HEAD"});
    }

    render();
}

void UpdatesView::finish_check(const QString& url, const QString& latest_commit, const QString& error) {
    checking_urls_.remove(url);
    if (latest_commit.isEmpty()) {
        latest_by_url_.remove(url);
        error_by_url_.insert(url, error);
    } else {
        latest_by_url_.insert(url, latest_commit);
        error_by_url_.remove(url);
    }

    for (auto& package : packages_) {
        if (package.url == url) {
            package.latest_commit = latest_commit;
            package.error = error;
            package.status = status_for(package);
        }
    }

    pending_checks_--;
    if (pending_checks_ == 0) {
        last_checked_ = QDateTime::currentDateTime();
    }
    render();
}

UpdatesView::Status UpdatesView::status_for(const Package& package) const {
    if (checking_urls_.contains(package.url)) {
        return Status::Checking;
    }
    if (!package.error.isEmpty()) {
        return Status::Failed;
    }
    if (package.latest_commit.isEmpty()) {
        return Status::NotChecked;
    }
    if (package.installed_commit.isEmpty()) {
        return Status::Unknown;
    }
    return package.installed_commit == package.latest_commit ? Status::UpToDate : Status::UpdateAvailable;
}

void UpdatesView::render() {
    content_stack_->setCurrentIndex(packages_.empty() ? 1 : 0);

    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    QColor secondary = palette().color(QPalette::PlaceholderText);
    bool running = update_process_ != nullptr;

    table_->clearContents();
    table_->setRowCount(static_cast<int>(packages_.size()));

    for (int row = 0; row < static_cast<int>(packages_.size()); row++) {
        const Package& package = packages_[row];

        QTableWidgetItem* name_item = new QTableWidgetItem(package.name);
        name_item->setToolTip(package.url);
        table_->setItem(row, NameColumn, name_item);

        QTableWidgetItem* installed_item = new QTableWidgetItem(package.installed_commit.isEmpty() ? "unknown" : package.installed_commit.left(7));
        installed_item->setFont(mono);
        installed_item->setToolTip(package.installed_commit);
        table_->setItem(row, InstalledColumn, installed_item);

        QTableWidgetItem* latest_item = new QTableWidgetItem(package.latest_commit.isEmpty() ? "—" : package.latest_commit.left(7));
        latest_item->setFont(mono);
        latest_item->setToolTip(package.latest_commit);
        table_->setItem(row, LatestColumn, latest_item);

        // Status in the theme's success, warning or error colour
        bool updating = running && (updating_name_ == "*" || updating_name_ == package.name);
        QString status_text;
        QColor status_color = secondary;
        if (updating) {
            status_text = "Updating…";
        } else {
            switch (package.status) {
                case Status::NotChecked:
                    status_text = "Not checked yet";
                    break;
                case Status::Checking:
                    status_text = "Checking…";
                    break;
                case Status::UpToDate:
                    status_text = "Up to date";
                    status_color = theme::success_color();
                    break;
                case Status::UpdateAvailable:
                    status_text = "Update available";
                    status_color = theme::warning_color();
                    break;
                case Status::Unknown:
                    status_text = "Installed commit unknown";
                    break;
                case Status::Failed:
                    status_text = "Couldn't check";
                    status_color = theme::error_color();
                    break;
            }
        }
        QTableWidgetItem* status_item = new QTableWidgetItem(status_text);
        status_item->setForeground(status_color);
        status_item->setToolTip(package.error);
        table_->setItem(row, StatusColumn, status_item);

        // Update button for packages that have, or may have, something new
        table_->removeCellWidget(row, ActionColumn);
        if (package.status == Status::UpdateAvailable || package.status == Status::Unknown) {
            QWidget* cell = new QWidget(table_);
            QHBoxLayout* cell_layout = new QHBoxLayout(cell);
            cell_layout->setContentsMargins(8, 4, 8, 4);
            QPushButton* button = new QPushButton("Update", cell);
            button->setStyleSheet(row_button_style());
            button->setEnabled(!running);
            QString name = package.name;
            QObject::connect(button, &QPushButton::clicked, this, [this, name]() { run_update(name); });
            cell_layout->addWidget(button);
            table_->setCellWidget(row, ActionColumn, cell);
        }
    }

    update_summary();
}

void UpdatesView::update_summary() {
    bool running = update_process_ != nullptr;
    int updates = 0;
    int failed = 0;
    for (const auto& package : packages_) {
        if (package.status == Status::UpdateAvailable || package.status == Status::Unknown) {
            updates++;
        } else if (package.status == Status::Failed) {
            failed++;
        }
    }
    int total = static_cast<int>(packages_.size());

    QString summary;
    if (running) {
        summary = updating_name_ == "*" ? "Updating all packages…" : "Updating " + updating_name_ + "…";
    } else if (packages_.empty()) {
        summary = "No packages installed from Git";
    } else if (pending_checks_ > 0) {
        summary = QString("Checking %1 %2 for updates…").arg(total).arg(total == 1 ? "package" : "packages");
    } else if (!last_checked_.isValid()) {
        summary = "Not checked yet";
    } else {
        if (updates > 0) {
            summary = QString("%1 %2 available").arg(updates).arg(updates == 1 ? "update" : "updates");
        } else if (failed == total) {
            summary = "Couldn't check for updates";
        } else {
            summary = "Everything is up to date";
        }
        if (failed > 0 && failed < total) {
            summary += QString(" · %1 couldn't be checked").arg(failed);
        }
        summary += " · Checked at " + last_checked_.toString("HH:mm");
    }
    summary_label_->setText(summary);

    check_btn_->setEnabled(!running && pending_checks_ == 0 && !packages_.empty());
    update_all_btn_->setEnabled(!running && updates > 0);
}

void UpdatesView::log_to_terminal(const QString& message) {
    terminal_panel_->append("> " + message);
}

void UpdatesView::run_update(const QString& name) {
    if (update_process_) {
        return;
    }

    // "*" updates everything: 'makekeeper update' with no package name
    QStringList args = {"update"};
    if (name != "*") {
        args << name;
    }

    updating_name_ = name;
    update_process_ = new QProcess(this);
    update_process_->setProcessChannelMode(QProcess::MergedChannels);
    update_process_->setStandardInputFile(QProcess::nullDevice());

    QObject::connect(update_process_, &QProcess::readyReadStandardOutput, this, [this]() {
        terminal_panel_->append(QString::fromUtf8(update_process_->readAllStandardOutput()).trimmed());
    });

    auto finish = [this]() {
        update_process_->deleteLater();
        update_process_ = nullptr;
        updating_name_.clear();
        // The registry now has the new installed commits; the latest commits are still known
        load_packages();
        render();
    };

    QObject::connect(update_process_, &QProcess::finished, this, [this, finish](int exit_code, QProcess::ExitStatus exit_status) {
        if (exit_status == QProcess::CrashExit) {
            log_to_terminal("Command crashed");
        } else if (exit_code == 0) {
            log_to_terminal("Command completed successfully");
        } else {
            log_to_terminal("Command failed with exit code: " + QString::number(exit_code));
        }
        finish();
    });

    QObject::connect(update_process_, &QProcess::errorOccurred, this, [this, finish](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            log_to_terminal("Failed to start makekeeper");
            finish();
        }
    });

    // Run as the user; makekeeper asks for root through makekeeper-helper only for the steps that need it
    log_to_terminal("Running: makekeeper " + args.join(' '));
    update_process_->start(QCoreApplication::applicationFilePath(), args);
    render();
}

} // namespace views
} // namespace gui
} // namespace makekeeper
