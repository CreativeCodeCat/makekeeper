#include "registry.h"
#include "../theme.h"
#include "../../config.h"
#include "../../registry.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QFrame>
#include <QAbstractItemView>
#include <QColor>

namespace makekeeper {
namespace gui {
namespace views {

RegistryView::RegistryView(QWidget* parent) : BaseView(parent) {
    setup_ui();
    load_registry();
}

void RegistryView::setup_ui() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // Title
    QLabel* title = new QLabel("Registry", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(window-text);");
    layout->addWidget(title);

    // Registry table
    registry_table_ = new QTableWidget(this);
    registry_table_->setColumnCount(5);
    registry_table_->setHorizontalHeaderLabels({"Name", "URL", "Commit", "Install Date", "Files"});
    registry_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    registry_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    registry_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    registry_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    registry_table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    registry_table_->setColumnWidth(1, 400);
    registry_table_->setColumnWidth(2, 80);
    registry_table_->setColumnWidth(3, 200);
    registry_table_->setColumnWidth(4, 60);
    registry_table_->verticalHeader()->setVisible(false);
    registry_table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    registry_table_->setShowGrid(true);
    registry_table_->setStyleSheet(
        "QTableWidget { background-color: palette(base); color: palette(text); border: 1px solid palette(mid); border-radius: 4px; }"
        "QTableWidget::item { padding: 4px; border: none; }"
        "QTableWidget::item:selected { background-color: palette(highlight); color: palette(highlighted-text); }"
        "QHeaderView::section { background-color: palette(window); color: palette(window-text); padding: 6px 8px; border: none; border-bottom: 1px solid palette(mid); font-weight: bold; font-size: 11px; }"
    );
    registry_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    registry_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    registry_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    theme::remove_focus_frame(registry_table_);
    layout->addWidget(registry_table_);

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
    terminal_panel_->setText("Terminal Logs\n---------\n> Registry view loaded\n");
    
    terminal_frame_layout->addWidget(terminal_panel_);
    layout->addWidget(terminal_frame);
}

void RegistryView::load_registry() {
    terminal_panel_->clear();
    terminal_panel_->setText("Terminal Logs\n---------\n");
    
    auto& registry = registry::Registry::instance();
    registry.load();

    auto records = registry.list();
    
    // Filter out empty names
    std::vector<registry::Record> valid_records;
    for (const auto& record : records) {
        if (!record.name.empty()) {
            valid_records.push_back(record);
        }
    }

    registry_table_->setRowCount(valid_records.size());

    for (size_t i = 0; i < valid_records.size(); i++) {
        const auto& record = valid_records[i];
        
        registry_table_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(record.name)));
        registry_table_->item(i, 0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        QString url = QString::fromStdString(record.url);
        registry_table_->setItem(i, 1, new QTableWidgetItem(url));
        registry_table_->item(i, 1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        QString commit = QString::fromStdString(record.commit_hash);
        if (commit.length() > 7) {
            commit = commit.left(7);
        }
        registry_table_->setItem(i, 2, new QTableWidgetItem(commit));
        registry_table_->item(i, 2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        QString date = QString::fromStdString(record.install_date);
        if (date.length() > 19) {
            date = date.left(19);
        }
        registry_table_->setItem(i, 3, new QTableWidgetItem(date));
        registry_table_->item(i, 3)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        QString file_count = QString::number(record.files.size());
        registry_table_->setItem(i, 4, new QTableWidgetItem(file_count));
        registry_table_->item(i, 4)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    terminal_panel_->append("> Loaded " + QString::number(valid_records.size()) + " packages from registry");
}

void RegistryView::refresh() {
    load_registry();
}

} // namespace views
} // namespace gui
} // namespace makekeeper
