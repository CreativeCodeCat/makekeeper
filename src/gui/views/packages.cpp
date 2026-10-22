#include "packages.h"
#include "../theme.h"
#include "../../config.h"
#include "../../registry.h"
#include "../../utils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFrame>
#include <QAbstractItemView>
#include <Qt>
#include <QColor>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFontMetrics>
#include <QMessageBox>
#include <QSettings>
#include <QUrl>
#include <QRegularExpression>

namespace makekeeper {
namespace gui {
namespace views {

PackagesView::PackagesView(QWidget* parent) : BaseView(parent), process_(nullptr) {
    setup_ui();
    load_packages();
}

void PackagesView::refresh() {
    load_packages();
}

void PackagesView::setup_ui() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // Search bar
    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText("Search packages...");
    search_edit_->setStyleSheet(
        "QLineEdit { background-color: palette(base); color: palette(text); border: 1px solid palette(mid); border-radius: 4px; padding: 10px 15px; font-size: 12px; }"
        "QLineEdit:focus { border: 1px solid palette(highlight); }"
    );
    QObject::connect(search_edit_, &QLineEdit::textChanged, [this](const QString& text) {
        on_search_changed(text);
    });
    layout->addWidget(search_edit_);

    // Content split (table + details)
    QHBoxLayout* split_layout = new QHBoxLayout();
    split_layout->setSpacing(10);

    // Package table (left)
    QFrame* table_frame = new QFrame(this);
    table_frame->setStyleSheet("QFrame { background-color: transparent; }");
    QVBoxLayout* table_frame_layout = new QVBoxLayout(table_frame);
    table_frame_layout->setContentsMargins(0, 0, 0, 0);
    
    package_table_ = new QTableWidget(this);
    package_table_->setColumnCount(4);
    package_table_->setHorizontalHeaderLabels({"Package Name", "Installed", "Status", "Version"});
    package_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    package_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    package_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    package_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    package_table_->setColumnWidth(1, 70);
    package_table_->setColumnWidth(2, 110);
    package_table_->setColumnWidth(3, 150);
    package_table_->verticalHeader()->setVisible(false);
    package_table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    package_table_->setShowGrid(true);
    package_table_->setStyleSheet(
        "QTableWidget { background-color: palette(base); color: palette(text); border: 1px solid palette(mid); border-radius: 4px; }"
        "QTableWidget::item { padding: 4px; border: none; }"
        "QTableWidget::item:selected { background-color: palette(highlight); color: palette(highlighted-text); }"
        "QHeaderView::section { background-color: palette(window); color: palette(window-text); padding: 6px 8px; border: none; border-bottom: 1px solid palette(mid); font-weight: bold; font-size: 11px; }"
        "QHeaderView::section::indicator { width: 0px; border: none; background: none; }"
        "QHeaderView::up-arrow, QHeaderView::down-arrow { width: 0px; height: 0px; border: none; background: none; image: none; }"
    );
    package_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    package_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    package_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    theme::remove_focus_frame(package_table_);
    package_table_->setSortingEnabled(true);
    package_table_->sortByColumn(0, Qt::AscendingOrder);
    package_table_->horizontalHeader()->setSectionsClickable(false);
    QObject::connect(package_table_, &QTableWidget::itemSelectionChanged, [this]() {
        on_package_selected();
    });

    table_frame_layout->addWidget(package_table_);
    split_layout->addWidget(table_frame);

    // Details panel (right)
    split_layout->addWidget(create_details_panel());
    layout->addLayout(split_layout);

    // Terminal logs (bottom)
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

void PackagesView::load_packages() {
    // Disable sorting while loading
    package_table_->setSortingEnabled(false);
    
    // Load installed packages
    auto& registry = registry::Registry::instance();
    registry.load();

    auto records = registry.list();
    installed_names_.clear();
    for (const auto& record : records) {
        installed_names_.insert(QString::fromStdString(record.name));
    }

    // Load package catalog
    QFile file(config::CACHE_FILE);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject root = doc.object();
    QJsonArray packages = root["packages"].toArray();

    package_table_->setRowCount(packages.size());

    for (int i = 0; i < packages.size(); i++) {
        QJsonObject obj = packages[i].toObject();
        QString name = obj["name"].toString();
        QString version = obj.contains("latest_version") ? obj["latest_version"].toString() : "latest";
        QString description = obj["description"].toString();
        QString url = obj["url"].toString();

        package_table_->setItem(i, 0, new QTableWidgetItem(name));
        package_table_->item(i, 0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        bool is_installed = installed_names_.contains(name);
        package_table_->setItem(i, 1, new QTableWidgetItem(is_installed ? "Yes" : "No"));
        package_table_->item(i, 1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        
        QTableWidgetItem* status_item = new QTableWidgetItem(is_installed ? "Up to date" : "Not installed");
        status_item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        if (is_installed) {
            status_item->setForeground(theme::success_color());
        } else {
            status_item->setForeground(palette().color(QPalette::PlaceholderText));
        }
        package_table_->setItem(i, 2, status_item);

        package_table_->setItem(i, 3, new QTableWidgetItem(version));
        package_table_->item(i, 3)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        package_table_->item(i, 0)->setData(Qt::UserRole, description);
        package_table_->item(i, 0)->setData(Qt::UserRole + 1, url);
        package_table_->item(i, 0)->setData(Qt::UserRole + 2, obj["category"].toString());
    }
    
    // Re-enable sorting and sort by name
    package_table_->setSortingEnabled(true);
    package_table_->sortByColumn(0, Qt::AscendingOrder);

    // Keep the selected package selected after a reload, such as after installing it
    for (int row = 0; row < package_table_->rowCount(); row++) {
        if (package_table_->item(row, 0)->text() == selected_name_) {
            package_table_->selectRow(row);
            show_details(row);
            return;
        }
    }
    selected_name_.clear();
    details_stack_->setCurrentIndex(0);
}

void PackagesView::on_search_changed(const QString& text) {
    for (int i = 0; i < package_table_->rowCount(); ++i) {
        QTableWidgetItem* name_item = package_table_->item(i, 0);
        bool match = name_item && name_item->text().contains(text, Qt::CaseInsensitive);
        package_table_->setRowHidden(i, !match);
    }
    package_table_->viewport()->update();
}

QWidget* PackagesView::create_details_panel() {
    QFrame* frame = new QFrame(this);
    frame->setObjectName("detailsPanel");
    frame->setFixedWidth(300);
    frame->setStyleSheet("QFrame#detailsPanel { background-color: palette(base); border: 1px solid palette(mid); border-radius: 4px; }");

    QVBoxLayout* frame_layout = new QVBoxLayout(frame);
    frame_layout->setContentsMargins(0, 0, 0, 0);

    details_stack_ = new QStackedWidget(frame);
    frame_layout->addWidget(details_stack_);

    // Empty state
    QLabel* empty_label = new QLabel("Select a package to see its details", details_stack_);
    empty_label->setAlignment(Qt::AlignCenter);
    empty_label->setWordWrap(true);
    empty_label->setStyleSheet("font-size: 12px;");
    theme::set_secondary(empty_label);
    details_stack_->addWidget(empty_label);

    // Details page
    QWidget* page = new QWidget(details_stack_);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(0);

    auto section_title = [page](const QString& text) {
        QLabel* label = new QLabel(text.toUpper(), page);
        label->setStyleSheet("font-size: 10px; font-weight: bold;");
        theme::set_secondary(label);
        return label;
    };

    name_label_ = new QLabel(page);
    name_label_->setWordWrap(true);
    name_label_->setStyleSheet("color: palette(text); font-size: 20px; font-weight: bold;");
    layout->addWidget(name_label_);

    meta_label_ = new QLabel(page);
    meta_label_->setStyleSheet("font-size: 12px;");
    theme::set_secondary(meta_label_);
    layout->addSpacing(2);
    layout->addWidget(meta_label_);

    status_label_ = new QLabel(page);
    layout->addSpacing(12);
    layout->addWidget(status_label_, 0, Qt::AlignLeft);

    description_label_ = new QLabel(page);
    description_label_->setWordWrap(true);
    description_label_->setStyleSheet("color: palette(text); font-size: 13px;");
    layout->addSpacing(14);
    layout->addWidget(description_label_);

    QFrame* separator = new QFrame(page);
    separator->setObjectName("detailsSeparator");
    separator->setFixedHeight(1);
    separator->setStyleSheet("QFrame#detailsSeparator { background-color: palette(mid); border: none; }");
    layout->addSpacing(16);
    layout->addWidget(separator);

    layout->addSpacing(14);
    layout->addWidget(section_title("Repository"));
    repo_label_ = new QLabel(page);
    repo_label_->setTextFormat(Qt::RichText);
    repo_label_->setOpenExternalLinks(true);
    repo_label_->setStyleSheet("font-size: 12px;");
    layout->addSpacing(4);
    layout->addWidget(repo_label_);

    installed_title_ = section_title("Installed");
    installed_label_ = new QLabel(page);
    installed_label_->setWordWrap(true);
    installed_label_->setStyleSheet("color: palette(text); font-size: 12px;");
    layout->addSpacing(12);
    layout->addWidget(installed_title_);
    layout->addSpacing(4);
    layout->addWidget(installed_label_);

    layout->addStretch();

    install_btn_ = new QPushButton(page);
    QObject::connect(install_btn_, &QPushButton::clicked, this, [this]() { on_install_clicked(); });
    layout->addWidget(install_btn_);

    repo_btn_ = new QPushButton("Open Repository", page);
    repo_btn_->setStyleSheet(theme::secondary_button_style());
    QObject::connect(repo_btn_, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl(selected_url_));
    });
    layout->addSpacing(8);
    layout->addWidget(repo_btn_);

    details_stack_->addWidget(page);
    return frame;
}

void PackagesView::on_package_selected() {
    auto selected = package_table_->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    show_details(selected[0]->row());
}

void PackagesView::show_details(int row) {
    QTableWidgetItem* name_item = package_table_->item(row, 0);
    QString name = name_item->text();
    QString version = package_table_->item(row, 3)->text();
    QString description = name_item->data(Qt::UserRole).toString();
    QString url = name_item->data(Qt::UserRole + 1).toString();
    QString category = name_item->data(Qt::UserRole + 2).toString();
    bool is_installed = installed_names_.contains(name);

    selected_name_ = name;
    selected_url_ = url;

    name_label_->setText(name);

    QStringList meta;
    if (!version.isEmpty()) {
        meta << version;
    }
    if (!category.isEmpty()) {
        meta << category;
    }
    meta_label_->setText(meta.join("  ·  "));

    // Status badge in the theme's success or warning colour
    QColor status_color = is_installed ? theme::success_color() : theme::warning_color();
    status_label_->setText(is_installed ? "● Installed" : "○ Not installed");
    status_label_->setStyleSheet(QString(
        "QLabel { color: %1; border: 1px solid %1; border-radius: 9px; padding: 2px 9px; font-size: 11px; font-weight: 600; }")
        .arg(status_color.name()));

    description_label_->setText(description.isEmpty() ? "No description available." : description);

    // Show the repository without the scheme or .git suffix, shortened to fit the panel
    QString display_url = url;
    display_url.remove(QRegularExpression("^[a-z]+://")).remove(QRegularExpression("\\.git$"));
    display_url = QFontMetrics(repo_label_->font()).elidedText(display_url, Qt::ElideMiddle, 260);
    repo_label_->setText(QString("<a href='%1' style='color: %2; text-decoration: none;'>%3</a>")
        .arg(url.toHtmlEscaped(), theme::link_color().name(), display_url.toHtmlEscaped()));
    repo_label_->setToolTip(url);
    repo_btn_->setEnabled(!url.isEmpty());

    // Installed commit and date from the registry
    auto record = is_installed ? registry::Registry::instance().get(name.toStdString()) : std::nullopt;
    installed_title_->setVisible(record.has_value());
    installed_label_->setVisible(record.has_value());
    if (record) {
        QString commit = QString::fromStdString(record->commit_hash).left(7);
        QString date = QString::fromStdString(record->install_date);
        installed_label_->setText(commit.isEmpty() ? date : QString("%1  ·  commit %2").arg(date, commit));
    }

    install_btn_->setText(is_installed ? "Uninstall" : "Install");
    install_btn_->setStyleSheet(is_installed ? theme::danger_button_style() : theme::primary_button_style());
    install_btn_->setEnabled(process_ == nullptr);

    details_stack_->setCurrentIndex(1);
}

void PackagesView::on_install_clicked() {
    if (selected_name_.isEmpty() || process_) {
        return;
    }

    if (installed_names_.contains(selected_name_)) {
        auto answer = QMessageBox::question(this, "Uninstall " + selected_name_,
            QString("Uninstall %1? This runs its 'make uninstall' and removes the files MakeKeeper tracked for it.").arg(selected_name_));
        if (answer == QMessageBox::Yes) {
            run_command({"uninstall", selected_name_});
        }
        return;
    }

    // Respect the "Confirm before installing packages" setting
    QSettings settings(QString::fromStdString(utils::user_config_dir()) + "/settings.conf", QSettings::IniFormat);
    if (settings.value("confirm_install", true).toBool()) {
        auto answer = QMessageBox::question(this, "Install " + selected_name_,
            QString("Install %1 from %2?\n\nIts Makefile will run with root privileges, so only install software you trust.")
                .arg(selected_name_, selected_url_));
        if (answer != QMessageBox::Yes) {
            return;
        }
    }
    run_command({"install", selected_name_});
}

void PackagesView::log_to_terminal(const QString& message) {
    terminal_panel_->append("> " + message);
}

void PackagesView::run_command(const QStringList& args) {
    process_ = new QProcess(this);
    process_->setProcessChannelMode(QProcess::MergedChannels);
    install_btn_->setEnabled(false);

    QObject::connect(process_, &QProcess::readyReadStandardOutput, this, [this]() {
        terminal_panel_->append(QString::fromUtf8(process_->readAllStandardOutput()).trimmed());
    });

    auto finish = [this]() {
        process_->deleteLater();
        process_ = nullptr;
        install_btn_->setEnabled(true);
        load_packages();
    };

    QObject::connect(process_, &QProcess::finished, this, [this, finish](int exit_code, QProcess::ExitStatus exit_status) {
        if (exit_status == QProcess::CrashExit) {
            log_to_terminal("Command crashed");
        } else if (exit_code == 0) {
            log_to_terminal("Command completed successfully");
        } else {
            log_to_terminal("Command failed with exit code: " + QString::number(exit_code));
        }
        finish();
    });

    QObject::connect(process_, &QProcess::errorOccurred, this, [this, finish](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            log_to_terminal("Failed to start makekeeper");
            finish();
        }
    });

    // Run as the user; makekeeper asks for root through makekeeper-helper only for the steps that need it
    log_to_terminal("Running: makekeeper " + args.join(' '));
    process_->start(QCoreApplication::applicationFilePath(), args);
}

} // namespace views
} // namespace gui
} // namespace makekeeper
