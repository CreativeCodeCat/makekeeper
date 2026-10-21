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
#include <iostream>

namespace makekeeper {
namespace commands {

class MakekeeperWindow : public QMainWindow {
public:
    MakekeeperWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle(QString("MakeKeeper v%1 | Package Manager").arg(config::VERSION));
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
        dark_palette.setColor(QPalette::Window, QColor(53, 53, 53));
        dark_palette.setColor(QPalette::WindowText, Qt::white);
        dark_palette.setColor(QPalette::Base, QColor(25, 25, 25));
        dark_palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        dark_palette.setColor(QPalette::ToolTipBase, Qt::white);
        dark_palette.setColor(QPalette::ToolTipText, Qt::white);
        dark_palette.setColor(QPalette::Text, Qt::white);
        dark_palette.setColor(QPalette::Button, QColor(53, 53, 53));
        dark_palette.setColor(QPalette::ButtonText, Qt::white);
        dark_palette.setColor(QPalette::BrightText, Qt::red);
        dark_palette.setColor(QPalette::Link, QColor(42, 130, 218));
        dark_palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        dark_palette.setColor(QPalette::HighlightedText, Qt::black);
        QApplication::setPalette(dark_palette);
    }

    void setup_ui() {
        QWidget* central_widget = new QWidget(this);
        setCentralWidget(central_widget);

        QVBoxLayout* main_layout = new QVBoxLayout(central_widget);
        main_layout->setContentsMargins(10, 10, 10, 10);
        main_layout->setSpacing(10);

        // Top bar with title and search
        QHBoxLayout* top_layout = new QHBoxLayout();
        top_layout->setSpacing(15);

        QLabel* title_label = new QLabel(QString("MakeKeeper v%1").arg(config::VERSION), this);
        title_label->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff;");
        title_label->setFixedWidth(200);

        search_edit_ = new QLineEdit(this);
        search_edit_->setPlaceholderText("Search packages...");
        search_edit_->setMaximumWidth(400);
        search_edit_->setStyleSheet(
            "QLineEdit {"
            "   background-color: #353535;"
            "   color: #ffffff;"
            "   border: 1px solid #555555;"
            "   border-radius: 5px;"
            "   padding: 8px;"
            "}"
            "QLineEdit:focus {"
            "   border: 1px solid #2a82da;"
            "}"
        );
        QObject::connect(search_edit_, &QLineEdit::textChanged, [this](const QString& text) {
            on_search_changed(text);
        });

        top_layout->addWidget(title_label);
        top_layout->addStretch();
        top_layout->addWidget(search_edit_);

        // Main content with splitter
        QSplitter* splitter = new QSplitter(Qt::Vertical, this);
        splitter->setStyleSheet("QSplitter::handle { background-color: #353535; }");

        // Package table
        package_table_ = new QTableWidget(this);
        package_table_->setColumnCount(4);
        package_table_->setHorizontalHeaderLabels({"Package Name", "Version", "Installed", "Status"});
        package_table_->horizontalHeader()->setStretchLastSection(false);
        package_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        package_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
        package_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        package_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        package_table_->setColumnWidth(1, 150);
        package_table_->setColumnWidth(2, 100);
        package_table_->setColumnWidth(3, 120);

        package_table_->setStyleSheet(
            "QTableWidget {"
            "   background-color: #191919;"
            "   color: #ffffff;"
            "   gridline-color: #353535;"
            "   border: none;"
            "}"
            "QTableWidget::item {"
            "   padding: 8px;"
            "   text-align: left;"
            "}"
            "QTableWidget::item:selected {"
            "   background-color: #2a82da;"
            "   color: #ffffff;"
            "}"
            "QHeaderView::section {"
            "   background-color: #353535;"
            "   color: #ffffff;"
            "   padding: 8px;"
            "   border: none;"
            "   border-bottom: 1px solid #555555;"
            "   font-weight: bold;"
            "   text-align: left;"
            "}"
            "QScrollBar:vertical {"
            "   background-color: #353535;"
            "   width: 12px;"
            "}"
            "QScrollBar::handle:vertical {"
            "   background-color: #555555;"
            "   border-radius: 6px;"
            "}"
        );
        package_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        package_table_->setSelectionMode(QAbstractItemView::SingleSelection);
        package_table_->verticalHeader()->setVisible(false);
        package_table_->setShowGrid(false);
        QObject::connect(package_table_, &QTableWidget::itemSelectionChanged, [this]() {
            on_package_selected();
        });

        // Details panel
        details_panel_ = new QTextEdit(this);
        details_panel_->setReadOnly(true);
        details_panel_->setMaximumHeight(350);
        details_panel_->setStyleSheet(
            "QTextEdit {"
            "   background-color: #191919;"
            "   color: #ffffff;"
            "   border: none;"
            "   padding: 15px;"
            "}"
        );

        splitter->addWidget(package_table_);
        splitter->addWidget(details_panel_);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 0);

        // Status bar
        status_label_ = new QLabel("Connected to registry", this);
        status_label_->setStyleSheet("color: #888888; font-size: 12px; padding: 5px;");

        main_layout->addLayout(top_layout);
        main_layout->addWidget(splitter);
        main_layout->addWidget(status_label_);
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
    }

private:
    QLineEdit* search_edit_;
    QTableWidget* package_table_;
    QTextEdit* details_panel_;
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
