#include "packages.h"
#include "../theme.h"
#include "../../config.h"
#include "../../registry.h"
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

namespace makekeeper {
namespace gui {
namespace views {

PackagesView::PackagesView(QWidget* parent) : BaseView(parent) {
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
    QFrame* details_frame = new QFrame(this);
    details_frame->setFixedWidth(300);
    details_frame->setStyleSheet("QFrame { background-color: palette(base); border: 1px solid palette(mid); border-radius: 4px; }");
    
    QVBoxLayout* details_frame_layout = new QVBoxLayout(details_frame);
    details_frame_layout->setContentsMargins(0, 0, 0, 0);
    
    details_panel_ = new QTextEdit(this);
    details_panel_->setReadOnly(true);
    details_panel_->setFrameStyle(QFrame::NoFrame);
    details_panel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    details_panel_->setStyleSheet(
        "QTextEdit { background-color: palette(base); color: palette(text); border: none; padding: 0px; font-size: 12px; }"
    );
    
    details_frame_layout->addWidget(details_panel_);

    split_layout->addWidget(details_frame);
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
    }
    
    // Re-enable sorting and sort by name
    package_table_->setSortingEnabled(true);
    package_table_->sortByColumn(0, Qt::AscendingOrder);
}

void PackagesView::on_search_changed(const QString& text) {
    for (int i = 0; i < package_table_->rowCount(); ++i) {
        QTableWidgetItem* name_item = package_table_->item(i, 0);
        bool match = name_item && name_item->text().contains(text, Qt::CaseInsensitive);
        package_table_->setRowHidden(i, !match);
    }
    package_table_->viewport()->update();
}

void PackagesView::on_package_selected() {
    auto selected = package_table_->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    int row = selected[0]->row();
    QString name = package_table_->item(row, 0)->text();
    QString version = package_table_->item(row, 3)->text();
    QString description = package_table_->item(row, 0)->data(Qt::UserRole).toString();
    QString url = package_table_->item(row, 0)->data(Qt::UserRole + 1).toString();
    bool is_installed = installed_names_.contains(name);

    // Rich text can't use palette roles, so take every colour from the active theme
    QString text_color = theme::color_name(QPalette::Text);
    QString secondary_color = theme::color_name(QPalette::PlaceholderText);
    QString link_color = theme::color_name(QPalette::Link);
    QString status_color = (is_installed ? theme::success_color() : theme::warning_color()).name();
    QString status_text = is_installed ? "Installed" : "Not installed";

    QString action_buttons;
    if (is_installed) {
        action_buttons = QString(
            "<button style='background-color: " + theme::error_color().name() + "; color: white; padding: 10px 16px; border: none; border-radius: 4px; cursor: pointer; font-size: 12px; font-weight: 500;'>Uninstall</button>"
        );
    } else {
        action_buttons = QString(
            "<button style='background-color: " + theme::success_color().name() + "; color: white; padding: 10px 16px; border: none; border-radius: 4px; cursor: pointer; font-size: 12px; font-weight: 500;'>Install</button>"
        );
    }

    QString details = QString(
        "<div style='padding: 5px;'>"
        "<h2 style='color: %8; margin: 0 0 8px 0; font-size: 18px; font-weight: 600;'>%1 <span style='font-size: 13px; color: %9; font-weight: 400; margin-left: 8px;'>%2</span></h2>"
        "<div style='background-color: %11; padding: 8px 12px; border-radius: 4px; margin: 12px 0; color: %8; font-size: 12px; line-height: 1.4;'>%3</div>"
        "<div style='margin: 15px 0;'>"
        "<div style='color: %9; font-size: 11px; margin-bottom: 4px;'>Repository</div>"
        "<div style='color: %10; font-size: 12px;'><a href='%4' style='color: %10; text-decoration: none;'>%4</a></div>"
        "</div>"
        "<div style='margin: 12px 0;'>"
        "<div style='color: %9; font-size: 11px; margin-bottom: 4px;'>Status</div>"
        "<div style='color: %5; font-size: 12px; font-weight: 500;'>%6</div>"
        "</div>"
        "<div style='margin: 20px 0 10px 0;'>"
        "<div style='color: %9; font-size: 11px; font-weight: 600; margin-bottom: 8px; text-transform: uppercase; letter-spacing: 0.5px;'>Actions</div>"
        "<div style='display: flex; flex-direction: column; gap: 8px;'>%7</div>"
        "<button style='background-color: %12; color: %13; padding: 8px 12px; border: 1px solid %14; border-radius: 4px; cursor: pointer; font-size: 12px; margin-top: 8px;'>Documentation</button>"
        "</div>"
        "</div>"
    ).arg(name, version, description, url, status_color, status_text, action_buttons, text_color, secondary_color)
     .arg(link_color, theme::color_name(QPalette::AlternateBase), theme::color_name(QPalette::Button),
          theme::color_name(QPalette::ButtonText), theme::color_name(QPalette::Mid));

    details_panel_->setHtml(details);
    terminal_panel_->append(QString("> Selected: %1").arg(name));
}

} // namespace views
} // namespace gui
} // namespace makekeeper
