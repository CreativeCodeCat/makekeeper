#include "gui.h"
#include "../config.h"
#include "../registry.h"
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSplitter>
#include <QHeaderView>
#include <QScrollBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QPalette>
#include <QFont>
#include <QProcess>
#include <QButtonGroup>
#include <iostream>

namespace makekeeper {
namespace commands {

class MakekeeperWindow : public QMainWindow {
public:
    MakekeeperWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle(QString("MakeKeeper | Package Manager v%1").arg(config::VERSION));
        setFixedSize(1200, 800);

        // Apply dark theme
        apply_dark_theme();

        setup_ui();
        load_packages();
        load_installed();
    }

private:
    void apply_dark_theme() {
        QPalette dark_palette;
        dark_palette.setColor(QPalette::Window, QColor(30, 30, 30));
        dark_palette.setColor(QPalette::WindowText, Qt::white);
        dark_palette.setColor(QPalette::Base, QColor(25, 25, 25));
        dark_palette.setColor(QPalette::AlternateBase, QColor(35, 35, 35));
        dark_palette.setColor(QPalette::Text, Qt::white);
        dark_palette.setColor(QPalette::Button, QColor(45, 45, 45));
        dark_palette.setColor(QPalette::ButtonText, Qt::white);
        dark_palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        dark_palette.setColor(QPalette::HighlightedText, Qt::white);
        QApplication::setPalette(dark_palette);
    }

    void setup_ui() {
        QWidget* central_widget = new QWidget(this);
        setCentralWidget(central_widget);

        QHBoxLayout* main_layout = new QHBoxLayout(central_widget);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->setSpacing(0);

        // Left sidebar
        QVBoxLayout* sidebar_layout = new QVBoxLayout();
        sidebar_layout->setContentsMargins(0, 0, 0, 0);
        sidebar_layout->setSpacing(0);

        QWidget* sidebar = new QWidget(this);
        sidebar->setFixedWidth(200);
        sidebar->setStyleSheet(
            "background-color: #252525;"
            "border-right: 1px solid #353535;"
        );
        sidebar->setLayout(sidebar_layout);

        QLabel* sidebar_title = new QLabel("Navigation", this);
        sidebar_title->setStyleSheet(
            "color: #888888;"
            "font-size: 11px;"
            "font-weight: bold;"
            "padding: 15px 15px 10px 15px;"
            "text-transform: uppercase;"
        );
        sidebar_layout->addWidget(sidebar_title);

        sidebar_buttons_ = new QButtonGroup(this);
        sidebar_buttons_->setExclusive(true);

        QStringList nav_items = {"Overview", "Packages", "Updates", "Registry", "Settings"};
        for (int i = 0; i < nav_items.size(); i++) {
            QPushButton* btn = new QPushButton(nav_items[i], this);
            btn->setCheckable(true);
            btn->setStyleSheet(
                "QPushButton {"
                "   background-color: transparent;"
                "   color: #e0e0e0;"
                "   border: none;"
                "   padding: 12px 15px;"
                "   text-align: left;"
                "   font-size: 13px;"
                "}"
                "QPushButton:hover {"
                "   background-color: #353535;"
                "}"
                "QPushButton:checked {"
                "   background-color: #2a82da;"
                "   color: #ffffff;"
                "   font-weight: bold;"
                "}"
            );
            if (nav_items[i] == "Packages") {
                btn->setChecked(true);
            }
            sidebar_buttons_->addButton(btn, i);
            sidebar_layout->addWidget(btn);
        }

        sidebar_layout->addStretch();

        main_layout->addWidget(sidebar);

        // Main content area
        QVBoxLayout* content_layout = new QVBoxLayout();
        content_layout->setContentsMargins(20, 20, 20, 20);
        content_layout->setSpacing(15);

        // Header with search
        QHBoxLayout* header_layout = new QHBoxLayout();
        header_layout->setSpacing(15);

        QLabel* section_title = new QLabel("Packages", this);
        section_title->setStyleSheet(
            "font-size: 24px;"
            "font-weight: bold;"
            "color: #ffffff;"
        );

        search_edit_ = new QLineEdit(this);
        search_edit_->setPlaceholderText("🔍 Search...");
        search_edit_->setMaximumWidth(300);
        search_edit_->setStyleSheet(
            "QLineEdit {"
            "   background-color: #353535;"
            "   color: #ffffff;"
            "   border: 1px solid #555555;"
            "   border-radius: 6px;"
            "   padding: 8px 12px;"
            "   font-size: 12px;"
            "}"
            "QLineEdit:focus {"
            "   border: 1px solid #2a82da;"
            "}"
        );
        QObject::connect(search_edit_, &QLineEdit::textChanged, [this](const QString& text) {
            on_search_changed(text);
        });

        header_layout->addWidget(section_title);
        header_layout->addStretch();
        header_layout->addWidget(search_edit_);

        // Content splitter (left: packages + terminal, right: details)
        QSplitter* content_splitter = new QSplitter(Qt::Horizontal, this);
        content_splitter->setStyleSheet("QSplitter::handle { background-color: #353535; width: 1px; }");

        // Left side (packages + terminal)
        QWidget* left_widget = new QWidget(this);
        QVBoxLayout* left_layout = new QVBoxLayout(left_widget);
        left_layout->setSpacing(15);

        // Package table header with columns
        QHBoxLayout* table_header = new QHBoxLayout();
        table_header->setSpacing(0);

        QLabel* col_name = new QLabel("Package Name", this);
        col_name->setStyleSheet("color: #888888; font-size: 11px; font-weight: bold; padding: 8px;");
        col_name->setFixedWidth(250);

        QLabel* col_version = new QLabel("Version", this);
        col_version->setStyleSheet("color: #888888; font-size: 11px; font-weight: bold; padding: 8px;");
        col_version->setFixedWidth(120);

        QLabel* col_installed = new QLabel("Installed", this);
        col_installed->setStyleSheet("color: #888888; font-size: 11px; font-weight: bold; padding: 8px;");
        col_installed->setFixedWidth(80);

        QLabel* col_status = new QLabel("Status", this);
        col_status->setStyleSheet("color: #888888; font-size: 11px; font-weight: bold; padding: 8px;");
        col_status->setFixedWidth(120);

        table_header->addWidget(col_name);
        table_header->addWidget(col_version);
        table_header->addWidget(col_installed);
        table_header->addWidget(col_status);
        table_header->addStretch();

        // Package table
        package_table_ = new QTableWidget(this);
        package_table_->setColumnCount(4);
        package_table_->setHorizontalHeaderLabels({"Package Name", "Version", "Installed", "Status"});
        package_table_->horizontalHeader()->setStretchLastSection(false);
        package_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        package_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
        package_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        package_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        package_table_->setColumnWidth(0, 250);
        package_table_->setColumnWidth(1, 120);
        package_table_->setColumnWidth(2, 80);
        package_table_->setColumnWidth(3, 120);
        package_table_->verticalHeader()->setVisible(false);
        package_table_->setShowGrid(false);
        package_table_->setStyleSheet(
            "QTableWidget {"
            "   background-color: #1e1e1e;"
            "   color: #e0e0e0;"
            "   border: none;"
            "}"
            "QTableWidget::item {"
            "   padding: 10px;"
            "   border-bottom: 1px solid #333333;"
            "}"
            "QTableWidget::item:selected {"
            "   background-color: #2a82da;"
            "   color: #ffffff;"
            "}"
            "QHeaderView::section {"
            "   background-color: transparent;"
            "   border: none;"
            "}"
        );
        package_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        package_table_->setSelectionMode(QAbstractItemView::SingleSelection);
        QObject::connect(package_table_, &QTableWidget::itemSelectionChanged, [this]() {
            on_package_selected();
        });

        left_layout->addLayout(table_header);
        left_layout->addWidget(package_table_);

        // Terminal
        terminal_panel_ = new QTextEdit(this);
        terminal_panel_->setReadOnly(true);
        terminal_panel_->setMaximumHeight(150);
        terminal_panel_->setStyleSheet(
            "QTextEdit {"
            "   background-color: #0d0d0d;"
            "   color: #00ff00;"
            "   font-family: 'Consolas', 'Monospace';"
            "   font-size: 11px;"
            "   border: 1px solid #353535;"
            "   border-radius: 6px;"
            "   padding: 10px;"
            "}"
        );
        terminal_panel_->setText("▶ MakeKeeper Terminal Ready\n");

        left_layout->addWidget(terminal_panel_);

        // Details panel (right side)
        details_panel_ = new QTextEdit(this);
        details_panel_->setReadOnly(true);
        details_panel_->setStyleSheet(
            "QTextEdit {"
            "   background-color: #1e1e1e;"
            "   color: #e0e0e0;"
            "   border: 1px solid #353535;"
            "   border-radius: 6px;"
            "   padding: 20px;"
            "   font-size: 13px;"
            "}"
        );

        content_splitter->addWidget(left_widget);
        content_splitter->addWidget(details_panel_);
        content_splitter->setStretchFactor(0, 2);
        content_splitter->setStretchFactor(1, 1);

        content_layout->addLayout(header_layout);
        content_layout->addWidget(content_splitter);

        main_layout->addLayout(content_layout);
    }

    void load_packages() {
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
            package_table_->setItem(i, 1, new QTableWidgetItem(version));
            package_table_->item(i, 1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

            bool is_installed = installed_names_.contains(name);
            package_table_->setItem(i, 2, new QTableWidgetItem(is_installed ? "Yes" : "No"));
            package_table_->item(i, 2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            package_table_->setItem(i, 3, new QTableWidgetItem(is_installed ? "Up to date" : "Update"));
            package_table_->item(i, 3)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

            // Store package data in user data
            package_table_->item(i, 0)->setData(Qt::UserRole, description);
            package_table_->item(i, 0)->setData(Qt::UserRole + 1, url);
        }
    }

    void load_installed() {
        auto& registry = registry::Registry::instance();
        registry.load();

        auto records = registry.list();
        for (const auto& record : records) {
            installed_names_.insert(QString::fromStdString(record.name));
        }
    }

    void on_search_changed(const QString& text) {
        for (int i = 0; i < package_table_->rowCount(); ++i) {
            bool match = false;
            for (int j = 0; j < package_table_->columnCount(); ++j) {
                if (package_table_->item(i, j)->text().contains(text, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
            package_table_->setRowHidden(i, !match);
        }
    }

    void on_package_selected() {
        auto selected = package_table_->selectedItems();
        if (selected.isEmpty()) {
            return;
        }

        int row = selected[0]->row();
        QString name = package_table_->item(row, 0)->text();
        QString version = package_table_->item(row, 1)->text();
        QString description = package_table_->item(row, 0)->data(Qt::UserRole).toString();
        QString url = package_table_->item(row, 0)->data(Qt::UserRole + 1).toString();
        bool is_installed = installed_names_.contains(name);

        QString action_buttons;
        if (is_installed) {
            action_buttons = QString(
                "<button style='background-color: #28a745; color: white; padding: 8px 16px; border: none; border-radius: 4px; margin-right: 8px;'>Update</button>"
                "<button style='background-color: #6c757d; color: white; padding: 8px 16px; border: none; border-radius: 4px; margin-right: 8px;'>Uninstall</button>"
                "<button style='background-color: #6c757d; color: white; padding: 8px 16px; border: none; border-radius: 4px;'>Documentation</button>"
            );
        } else {
            action_buttons = QString(
                "<button style='background-color: #28a745; color: white; padding: 8px 16px; border: none; border-radius: 4px; margin-right: 8px;'>Install</button>"
                "<button style='background-color: #6c757d; color: white; padding: 8px 16px; border: none; border-radius: 4px;'>Documentation</button>"
            );
        }

        QString details = QString(
            "<h2 style='color: #ffffff; margin: 0 0 5px 0; font-size: 20px;'>%1 <span style='font-size: 14px; color: #888888; font-weight: normal;'>v%2</span></h2>"
            "<p style='color: #cccccc; margin: 10px 0;'>%3</p>"
            "<p style='color: #888888; margin: 10px 0;'><b>Repository:</b> <a href='%4' style='color: #2a82da;'>%4</a></p>"
            "<p style='color: #888888; margin: 10px 0;'><b>License:</b> <a href='#' style='color: #2a82da;'>MIT</a></p>"
            "<p style='color: #888888; margin: 10px 0;'><b>Dependencies:</b></p>"
            "<div style='margin: 5px 0;'>"
            "<span style='background-color: #353535; color: #e0e0e0; padding: 4px 8px; border-radius: 4px; margin-right: 5px; font-size: 11px;'>clap</span>"
            "<span style='background-color: #353535; color: #e0e0e0; padding: 4px 8px; border-radius: 4px; margin-right: 5px; font-size: 11px;'>serde</span>"
            "<span style='background-color: #353535; color: #e0e0e0; padding: 4px 8px; border-radius: 4px; font-size: 11px;'>tokio</span>"
            "</div>"
            "<p style='color: #888888; margin: 20px 0 10px 0;'><b>Actions:</b></p>"
            "<div style='margin-top: 10px;'>%5</div>"
        ).arg(name, version, description, url, action_buttons);

        details_panel_->setHtml(details);

        // Log to terminal
        terminal_panel_->append(QString("▶ Selected: %1 v%2").arg(name, version));
    }

private:
    QLineEdit* search_edit_;
    QTableWidget* package_table_;
    QTextEdit* details_panel_;
    QTextEdit* terminal_panel_;
    QButtonGroup* sidebar_buttons_;
    QSet<QString> installed_names_;
};

    void load_packages() {
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
            package_table_->setItem(i, 1, new QTableWidgetItem(version));
            package_table_->item(i, 1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

            bool is_installed = installed_names_.contains(name);
            package_table_->setItem(i, 2, new QTableWidgetItem(is_installed ? "Yes" : "No"));
            package_table_->item(i, 2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            package_table_->setItem(i, 3, new QTableWidgetItem(is_installed ? "Up to date" : "Update"));
            package_table_->item(i, 3)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

            // Store package data in user data
            package_table_->item(i, 0)->setData(Qt::UserRole, description);
            package_table_->item(i, 0)->setData(Qt::UserRole + 1, url);
        }
    }

    void load_installed() {
        auto& registry = registry::Registry::instance();
        registry.load();

        auto records = registry.list();
        for (const auto& record : records) {
            installed_names_.insert(QString::fromStdString(record.name));
        }
    }

    void on_search_changed(const QString& text) {
        for (int i = 0; i < package_table_->rowCount(); ++i) {
            bool match = false;
            for (int j = 0; j < package_table_->columnCount(); ++j) {
                if (package_table_->item(i, j)->text().contains(text, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
            package_table_->setRowHidden(i, !match);
        }
    }

    void on_package_selected() {
        auto selected = package_table_->selectedItems();
        if (selected.isEmpty()) {
            return;
        }

        int row = selected[0]->row();
        QString name = package_table_->item(row, 0)->text();
        QString version = package_table_->item(row, 1)->text();
        QString description = package_table_->item(row, 0)->data(Qt::UserRole).toString();
        QString url = package_table_->item(row, 0)->data(Qt::UserRole + 1).toString();
        bool is_installed = installed_names_.contains(name);

        QString action_buttons;
        if (is_installed) {
            action_buttons = QString(
                "<button style='background-color: #2a82da; color: white; padding: 8px 16px; border: none; border-radius: 4px; margin-right: 8px;'>Update</button>"
                "<button style='background-color: #d9534f; color: white; padding: 8px 16px; border: none; border-radius: 4px;'>Uninstall</button>"
            );
        } else {
            action_buttons = QString(
                "<button style='background-color: #2a82da; color: white; padding: 8px 16px; border: none; border-radius: 4px;'>Install</button>"
            );
        }

        QString details = QString(
            "<h2 style='color: #ffffff; margin-bottom: 10px;'>%1 v%2</h2>"
            "<p style='color: #cccccc; margin-bottom: 15px;'>%3</p>"
            "<p style='color: #888888; margin-bottom: 15px;'><b>Repository:</b> <a href='%4' style='color: #2a82da;'>%4</a></p>"
            "<p style='color: #888888; margin-bottom: 10px;'><b>Status:</b> %5</p>"
            "<p style='color: #888888; margin-bottom: 5px;'><b>Actions:</b></p>"
            "<div style='margin-top: 10px;'>%6</div>"
        ).arg(name, version, description, url, is_installed ? "Installed" : "Not installed", action_buttons);

        details_panel_->setHtml(details);

        // Log to terminal
        terminal_panel_->append(QString("▶ Selected: %1 v%2").arg(name, version));
    }

private:
    QLineEdit* search_edit_;
    QTableWidget* package_table_;
    QTextEdit* details_panel_;
    QTextEdit* terminal_panel_;
    QLabel* status_label_;
    QSet<QString> installed_names_;
};

int gui(const std::vector<std::string>& args) {
    int argc = 0;
    char** argv = nullptr;

    QApplication app(argc, argv);

    MakekeeperWindow window;
    window.show();

    return app.exec();
}

} // namespace commands
} // namespace makekeeper
