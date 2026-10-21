#include "overview.h"
#include "../../config.h"
#include "../../registry.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QFont>
#include <QPushButton>
#include <QFile>
#include <QObject>
#include <QProcess>
#include <QCoreApplication>

namespace makekeeper {
namespace gui {
namespace views {

OverviewView::OverviewView(QWidget* parent) : BaseView(parent), process_(nullptr) {
    setup_ui();
    update_stats();
}

void OverviewView::setup_ui() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(15);

    // Title
    QLabel* title = new QLabel("Overview", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    layout->addWidget(title);

    // Stats grid
    QHBoxLayout* stats_layout = new QHBoxLayout();
    stats_layout->setSpacing(15);

    // Total installed card
    QFrame* installed_card = new QFrame(this);
    installed_card->setStyleSheet("QFrame { background-color: #1e1e1e; border: 1px solid #353535; border-radius: 8px; }");
    QVBoxLayout* installed_layout = new QVBoxLayout(installed_card);
    installed_layout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* installed_label = new QLabel("Total Installed", this);
    installed_label->setStyleSheet("color: #888888; font-size: 12px; font-weight: 500;");
    installed_layout->addWidget(installed_label);
    
    total_installed_label_ = new QLabel("0", this);
    total_installed_label_->setStyleSheet("color: #2a82da; font-size: 36px; font-weight: bold;");
    installed_layout->addWidget(total_installed_label_);
    
    installed_layout->addStretch();
    stats_layout->addWidget(installed_card);

    // Git repos card
    QFrame* git_card = new QFrame(this);
    git_card->setStyleSheet("QFrame { background-color: #1e1e1e; border: 1px solid #353535; border-radius: 8px; }");
    QVBoxLayout* git_layout = new QVBoxLayout(git_card);
    git_layout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* git_label = new QLabel("Git Repositories", this);
    git_label->setStyleSheet("color: #888888; font-size: 12px; font-weight: 500;");
    git_layout->addWidget(git_label);
    
    git_repos_label_ = new QLabel("0", this);
    git_repos_label_->setStyleSheet("color: #66cc66; font-size: 36px; font-weight: bold;");
    git_layout->addWidget(git_repos_label_);
    
    git_layout->addStretch();
    stats_layout->addWidget(git_card);

    // Sync status card
    QFrame* sync_card = new QFrame(this);
    sync_card->setStyleSheet("QFrame { background-color: #1e1e1e; border: 1px solid #353535; border-radius: 8px; }");
    QVBoxLayout* sync_layout = new QVBoxLayout(sync_card);
    sync_layout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* sync_label = new QLabel("Catalog Status", this);
    sync_label->setStyleSheet("color: #888888; font-size: 12px; font-weight: 500;");
    sync_layout->addWidget(sync_label);
    
    sync_status_label_ = new QLabel("Checking...", this);
    sync_status_label_->setStyleSheet("color: #e0e0e0; font-size: 14px; font-weight: 500;");
    sync_layout->addWidget(sync_status_label_);
    
    sync_layout->addStretch();
    stats_layout->addWidget(sync_card);

    stats_layout->addStretch();
    layout->addLayout(stats_layout);

    // Quick actions section
    QLabel* actions_title = new QLabel("Quick Actions", this);
    actions_title->setStyleSheet("color: #e0e0e0; font-size: 16px; font-weight: bold;");
    layout->addWidget(actions_title);

    QHBoxLayout* actions_layout = new QHBoxLayout();
    actions_layout->setSpacing(10);

    QPushButton* sync_btn = new QPushButton("Sync Catalog", this);
    sync_btn->setStyleSheet(
        "QPushButton { background-color: #2a82da; color: #ffffff; border: none; border-radius: 4px; padding: 10px 20px; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background-color: #3a92ea; }"
        "QPushButton:pressed { background-color: #1a72ca; }"
    );
    QObject::connect(sync_btn, &QPushButton::clicked, [this]() {
        run_command("sync");
    });
    actions_layout->addWidget(sync_btn);

    QPushButton* update_all_btn = new QPushButton("Update All", this);
    update_all_btn->setStyleSheet(
        "QPushButton { background-color: #353535; color: #e0e0e0; border: 1px solid #444444; border-radius: 4px; padding: 10px 20px; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background-color: #454545; }"
        "QPushButton:pressed { background-color: #252525; }"
    );
    QObject::connect(update_all_btn, &QPushButton::clicked, [this]() {
        run_command("update");
    });
    actions_layout->addWidget(update_all_btn);

    actions_layout->addStretch();
    layout->addLayout(actions_layout);

    // Terminal logs
    QFrame* terminal_frame = new QFrame(this);
    terminal_frame->setMaximumHeight(140);
    terminal_frame->setStyleSheet("QFrame { background-color: #1e1e1e; border: 1px solid #353535; border-radius: 4px; }");
    
    QVBoxLayout* terminal_frame_layout = new QVBoxLayout(terminal_frame);
    terminal_frame_layout->setContentsMargins(0, 0, 0, 0);
    
    terminal_panel_ = new QTextEdit(this);
    terminal_panel_->setReadOnly(true);
    terminal_panel_->setFrameStyle(QFrame::NoFrame);
    terminal_panel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    terminal_panel_->setStyleSheet(
        "QTextEdit { background-color: #1e1e1e; color: #66ff66; font-family: 'Consolas', 'Monospace'; font-size: 11px; border: none; padding: 10px; }"
    );
    terminal_panel_->setText("Terminal Logs\n---------\n> MakeKeeper Terminal Ready\n");
    
    terminal_frame_layout->addWidget(terminal_panel_);
    layout->addWidget(terminal_frame);
}

void OverviewView::log_to_terminal(const QString& message) {
    terminal_panel_->append("> " + message);
}

void OverviewView::run_command(const QString& command) {
    if (process_) {
        log_to_terminal("Command already running...");
        return;
    }

    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(process_, &QProcess::readyReadStandardOutput, [this]() {
        QString output = QString::fromUtf8(process_->readAllStandardOutput());
        terminal_panel_->append(output.trimmed());
    });

    QObject::connect(process_, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [this](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitCode == 0) {
                log_to_terminal("Command completed successfully");
            } else {
                log_to_terminal("Command failed with exit code: " + QString::number(exitCode));
            }
            process_->deleteLater();
            process_ = nullptr;
            update_stats();
        });

    // Get the makekeeper binary path
    QString binary_path = QCoreApplication::applicationFilePath();
    QStringList args = command.split(' ', Qt::SkipEmptyParts);
    
    // Commands that need pkexec
    bool needs_privilege = (command == "sync" || command == "update");
    
    if (needs_privilege) {
        log_to_terminal("Running: pkexec makekeeper " + args.join(' '));
        QStringList pkexec_args;
        pkexec_args << binary_path << args;
        process_->start("pkexec", pkexec_args);
    } else {
        log_to_terminal("Running: makekeeper " + args.join(' '));
        process_->start(binary_path, args);
    }
}

void OverviewView::update_stats() {
    auto& registry = registry::Registry::instance();
    registry.load();

    auto records = registry.list();
    int total_installed = 0;
    int git_repos = 0;

    for (const auto& record : records) {
        if (!record.name.empty()) {
            total_installed++;
            if (!record.url.empty()) {
                git_repos++;
            }
        }
    }

    total_installed_label_->setText(QString::number(total_installed));
    git_repos_label_->setText(QString::number(git_repos));

    // Check if catalog exists
    QFile catalog_file(config::CACHE_FILE);
    if (catalog_file.exists()) {
        sync_status_label_->setText("Synced");
        sync_status_label_->setStyleSheet("color: #66cc66; font-size: 14px; font-weight: 500;");
    } else {
        sync_status_label_->setText("Not Synced");
        sync_status_label_->setStyleSheet("color: #cc6666; font-size: 14px; font-weight: 500;");
    }
}

void OverviewView::refresh() {
    update_stats();
}

} // namespace views
} // namespace gui
} // namespace makekeeper
