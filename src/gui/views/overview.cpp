#include "overview.h"
#include "../../config.h"
#include "../../utils.h"
#include "../../registry.h"
#include "../theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QSettings>

namespace makekeeper {
namespace gui {
namespace views {

namespace {

// Selectors use object names so card styles don't leak into the labels inside them (QLabel is a QFrame)
const char* CARD_STYLE =
    "QFrame#statCard { background-color: palette(base); border: 1px solid palette(mid); border-radius: 8px; }";
const char* CARD_TITLE_STYLE = "font-size: 12px; font-weight: 500;";
const char* CARD_VALUE_STYLE = "color: palette(highlight); font-size: 32px; font-weight: bold;";
const char* CARD_DETAIL_STYLE = "font-size: 11px;";

QString status_style(const QColor& color) {
    return QString("color: %1; font-size: 20px; font-weight: bold;").arg(color.name());
}

} // namespace

OverviewView::OverviewView(QWidget* parent) : BaseView(parent), process_(nullptr) {
    setup_ui();
    update_stats();
    check_auto_sync();
}

QWidget* OverviewView::create_stat_card(const QString& title, QLabel* value, QLabel* detail) {
    QFrame* card = new QFrame(this);
    card->setObjectName("statCard");
    card->setStyleSheet(CARD_STYLE);

    QVBoxLayout* card_layout = new QVBoxLayout(card);
    card_layout->setContentsMargins(20, 16, 20, 16);
    card_layout->setSpacing(4);

    QLabel* title_label = new QLabel(title, card);
    title_label->setStyleSheet(CARD_TITLE_STYLE);
    theme::set_secondary(title_label);
    card_layout->addWidget(title_label);

    value->setParent(card);
    card_layout->addWidget(value);

    detail->setParent(card);
    detail->setStyleSheet(CARD_DETAIL_STYLE);
    theme::set_secondary(detail);
    card_layout->addWidget(detail);

    card_layout->addStretch();
    return card;
}

void OverviewView::setup_ui() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(15);

    // Title
    QLabel* title = new QLabel("Overview", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(window-text);");
    layout->addWidget(title);

    // Stat cards, sharing the width equally
    total_installed_label_ = new QLabel("0");
    total_installed_label_->setStyleSheet(CARD_VALUE_STYLE);
    total_installed_detail_ = new QLabel();

    git_repos_label_ = new QLabel("0");
    git_repos_label_->setStyleSheet(CARD_VALUE_STYLE);
    git_repos_detail_ = new QLabel();

    sync_status_label_ = new QLabel("Checking...");
    sync_status_detail_ = new QLabel();

    QHBoxLayout* stats_layout = new QHBoxLayout();
    stats_layout->setSpacing(15);
    stats_layout->addWidget(create_stat_card("Installed Packages", total_installed_label_, total_installed_detail_), 1);
    stats_layout->addWidget(create_stat_card("From Git", git_repos_label_, git_repos_detail_), 1);
    stats_layout->addWidget(create_stat_card("Package Catalog", sync_status_label_, sync_status_detail_), 1);
    layout->addLayout(stats_layout);

    // Quick actions
    QLabel* actions_title = new QLabel("Quick Actions", this);
    actions_title->setStyleSheet("color: palette(window-text); font-size: 16px; font-weight: bold;");
    layout->addWidget(actions_title);

    QHBoxLayout* actions_layout = new QHBoxLayout();
    actions_layout->setSpacing(10);

    sync_btn_ = new QPushButton("Sync Catalog", this);
    sync_btn_->setToolTip("Download the latest package catalog");
    sync_btn_->setStyleSheet(theme::primary_button_style());
    QObject::connect(sync_btn_, &QPushButton::clicked, this, [this]() { run_command("sync"); });
    actions_layout->addWidget(sync_btn_);

    update_all_btn_ = new QPushButton("Update All", this);
    update_all_btn_->setToolTip("Pull, rebuild and reinstall every package installed from Git");
    update_all_btn_->setStyleSheet(theme::secondary_button_style());
    QObject::connect(update_all_btn_, &QPushButton::clicked, this, [this]() { run_command("update"); });
    actions_layout->addWidget(update_all_btn_);

    actions_layout->addStretch();
    layout->addLayout(actions_layout);

    // Terminal logs, filling the rest of the page
    QFrame* terminal_frame = new QFrame(this);
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
    layout->addWidget(terminal_frame, 1);
}

void OverviewView::log_to_terminal(const QString& message) {
    terminal_panel_->append("> " + message);
}

void OverviewView::check_auto_sync() {
    QString config_path = QString::fromStdString(utils::user_config_dir()) + "/settings.conf";
    QSettings settings(config_path, QSettings::IniFormat);

    if (settings.value("auto_sync", false).toBool()) {
        log_to_terminal("Auto-sync enabled, running sync...");
        run_command("sync");
    }
}

void OverviewView::set_running(bool running) {
    sync_btn_->setEnabled(!running);
    update_all_btn_->setEnabled(!running);
}

void OverviewView::run_command(const QString& command) {
    if (process_) {
        return;
    }

    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);
    set_running(true);

    QObject::connect(process_, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = QString::fromUtf8(process_->readAllStandardOutput());
        terminal_panel_->append(output.trimmed());
    });

    QObject::connect(process_, &QProcess::finished, this, [this](int exit_code, QProcess::ExitStatus exit_status) {
        if (exit_status == QProcess::CrashExit) {
            log_to_terminal("Command crashed");
        } else if (exit_code == 0) {
            log_to_terminal("Command completed successfully");
        } else {
            log_to_terminal("Command failed with exit code: " + QString::number(exit_code));
        }
        process_->deleteLater();
        process_ = nullptr;
        set_running(false);
        update_stats();
    });

    QObject::connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            log_to_terminal("Failed to start makekeeper");
            process_->deleteLater();
            process_ = nullptr;
            set_running(false);
        }
    });

    // Run as the user; makekeeper asks for root through makekeeper-helper only for the steps that need it
    QStringList args = command.split(' ', Qt::SkipEmptyParts);
    log_to_terminal("Running: makekeeper " + args.join(' '));
    process_->start(QCoreApplication::applicationFilePath(), args);
}

void OverviewView::update_stats() {
    auto& registry = registry::Registry::instance();
    registry.load();

    int total_installed = 0;
    int git_repos = 0;
    for (const auto& record : registry.list()) {
        if (record.name.empty()) {
            continue;
        }
        total_installed++;
        if (!record.url.empty()) {
            git_repos++;
        }
    }

    total_installed_label_->setText(QString::number(total_installed));
    total_installed_detail_->setText(total_installed == 1 ? "package tracked" : "packages tracked");
    git_repos_label_->setText(QString::number(git_repos));
    git_repos_detail_->setText("can be updated with Update All");

    // Catalog status: whether it has been synced, its version and when
    QFile catalog_file(config::CACHE_FILE);
    if (!catalog_file.open(QIODevice::ReadOnly)) {
        sync_status_label_->setText("Not Synced");
        sync_status_label_->setStyleSheet(status_style(theme::error_color()));
        sync_status_detail_->setText("Run Sync Catalog to download it");
        return;
    }

    QString version = QJsonDocument::fromJson(catalog_file.readAll()).object().value("version").toString();
    QString updated = QFileInfo(catalog_file).lastModified().toString("yyyy-MM-dd HH:mm");

    sync_status_label_->setText("Synced");
    sync_status_label_->setStyleSheet(status_style(theme::success_color()));
    sync_status_detail_->setText(version.isEmpty() ? "Updated " + updated : "Version " + version + " · updated " + updated);
}

void OverviewView::refresh() {
    update_stats();
}

} // namespace views
} // namespace gui
} // namespace makekeeper
