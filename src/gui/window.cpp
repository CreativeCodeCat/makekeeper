#include "window.h"
#include "../config.h"
#include <QApplication>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QFrame>

namespace makekeeper {
namespace gui {

MakekeeperWindow::MakekeeperWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QString("MakeKeeper | Package Manager v%1").arg(config::VERSION));
    setFixedSize(1100, 650);

    apply_dark_theme();
    setup_ui();
}

MakekeeperWindow::~MakekeeperWindow() {
    // Qt automatically deletes child widgets, but we can be explicit
    delete content_stack_;
    delete nav_overview_;
    delete nav_packages_;
    delete nav_updates_;
    delete nav_registry_;
    delete nav_settings_;
}

void MakekeeperWindow::apply_dark_theme() {
    QPalette breeze_dark;

    // Window backgrounds
    breeze_dark.setColor(QPalette::Window, QColor("#232629"));
    breeze_dark.setColor(QPalette::WindowText, QColor("#FCFCFC"));

    // Input fields and text editors
    breeze_dark.setColor(QPalette::Base, QColor("#1B1E20"));
    breeze_dark.setColor(QPalette::AlternateBase, QColor("#272B2F"));
    breeze_dark.setColor(QPalette::Text, QColor("#FCFCFC"));

    // Buttons
    breeze_dark.setColor(QPalette::Button, QColor("#31363B"));
    breeze_dark.setColor(QPalette::ButtonText, QColor("#FCFCFC"));

    // Breeze blue accent
    breeze_dark.setColor(QPalette::Highlight, QColor("#3DAEE9"));
    breeze_dark.setColor(QPalette::HighlightedText, QColor("#FCFCFC"));

    // Tooltips
    breeze_dark.setColor(QPalette::ToolTipBase, QColor("#31363B"));
    breeze_dark.setColor(QPalette::ToolTipText, QColor("#FCFCFC"));

    // Disabled widgets
    breeze_dark.setColor(QPalette::Disabled, QPalette::WindowText,
                         QColor("#7F858B"));
    breeze_dark.setColor(QPalette::Disabled, QPalette::Text,
                         QColor("#7F858B"));
    breeze_dark.setColor(QPalette::Disabled, QPalette::ButtonText,
                         QColor("#7F858B"));

    // Links
    breeze_dark.setColor(QPalette::Link, QColor("#3DAEE9"));
    breeze_dark.setColor(QPalette::LinkVisited, QColor("#9B59B6"));

    // Mid color for borders
    breeze_dark.setColor(QPalette::Mid, QColor("#353535"));

    QApplication::setPalette(breeze_dark);

    QStyle *style = QStyleFactory::create("Breeze");

    if (style) {
        QApplication::setStyle(style);
    } else {
        qWarning() << "Breeze style is not available";
    }
}

void MakekeeperWindow::setup_ui() {
    QWidget* central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    QHBoxLayout* main_layout = new QHBoxLayout(central_widget);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // Sidebar
    QWidget* sidebar = new QWidget(this);
    sidebar->setFixedWidth(140);
    sidebar->setStyleSheet("background-color: palette(window); border-right: 1px solid palette(mid);");

    QVBoxLayout* sidebar_layout = new QVBoxLayout(sidebar);
    sidebar_layout->setContentsMargins(0, 0, 0, 0);
    sidebar_layout->setSpacing(0);

    nav_overview_ = new QPushButton("Overview", this);
    nav_overview_->setCheckable(true);
    nav_overview_->setChecked(true);
    nav_overview_->setStyleSheet(
        "QPushButton { background-color: transparent; color: palette(window-text); border: none; padding: 8px 12px; text-align: left; font-size: 13px; }"
        "QPushButton:hover { background-color: palette(button); }"
        "QPushButton:checked { background-color: palette(highlight); color: palette(highlighted-text); font-weight: bold; }"
    );
    QObject::connect(nav_overview_, &QPushButton::clicked, [this]() { on_nav_clicked(0); });
    sidebar_layout->addWidget(nav_overview_);

    nav_packages_ = new QPushButton("Packages", this);
    nav_packages_->setCheckable(true);
    nav_packages_->setStyleSheet(
        "QPushButton { background-color: transparent; color: palette(window-text); border: none; padding: 8px 12px; text-align: left; font-size: 13px; }"
        "QPushButton:hover { background-color: palette(button); }"
        "QPushButton:checked { background-color: palette(highlight); color: palette(highlighted-text); font-weight: bold; }"
    );
    QObject::connect(nav_packages_, &QPushButton::clicked, [this]() { on_nav_clicked(1); });
    sidebar_layout->addWidget(nav_packages_);

    nav_updates_ = new QPushButton("Updates", this);
    nav_updates_->setCheckable(true);
    nav_updates_->setStyleSheet(
        "QPushButton { background-color: transparent; color: palette(window-text); border: none; padding: 8px 12px; text-align: left; font-size: 13px; }"
        "QPushButton:hover { background-color: palette(button); }"
        "QPushButton:checked { background-color: palette(highlight); color: palette(highlighted-text); font-weight: bold; }"
    );
    QObject::connect(nav_updates_, &QPushButton::clicked, [this]() { on_nav_clicked(2); });
    sidebar_layout->addWidget(nav_updates_);

    nav_registry_ = new QPushButton("Registry", this);
    nav_registry_->setCheckable(true);
    nav_registry_->setStyleSheet(
        "QPushButton { background-color: transparent; color: palette(window-text); border: none; padding: 8px 12px; text-align: left; font-size: 13px; }"
        "QPushButton:hover { background-color: palette(button); }"
        "QPushButton:checked { background-color: palette(highlight); color: palette(highlighted-text); font-weight: bold; }"
    );
    QObject::connect(nav_registry_, &QPushButton::clicked, [this]() { on_nav_clicked(3); });
    sidebar_layout->addWidget(nav_registry_);

    nav_settings_ = new QPushButton("Settings", this);
    nav_settings_->setCheckable(true);
    nav_settings_->setStyleSheet(
        "QPushButton { background-color: transparent; color: palette(window-text); border: none; padding: 8px 12px; text-align: left; font-size: 13px; }"
        "QPushButton:hover { background-color: palette(button); }"
        "QPushButton:checked { background-color: palette(highlight); color: palette(highlighted-text); font-weight: bold; }"
    );
    QObject::connect(nav_settings_, &QPushButton::clicked, [this]() { on_nav_clicked(4); });
    sidebar_layout->addWidget(nav_settings_);

    sidebar_layout->addStretch();
    main_layout->addWidget(sidebar);

    // Main content area
    QWidget* main_content = new QWidget(this);
    QVBoxLayout* main_content_layout = new QVBoxLayout(main_content);
    main_content_layout->setContentsMargins(0, 0, 0, 0);
    main_content_layout->setSpacing(0);

    // Content stack
    content_stack_ = new QStackedWidget(this);
    
    // Add views
    content_stack_->addWidget(new views::OverviewView(this));
    content_stack_->addWidget(new views::PackagesView(this));
    content_stack_->addWidget(new views::UpdatesView(this));
    content_stack_->addWidget(new views::RegistryView(this));
    content_stack_->addWidget(new views::SettingsView(this));
    
    content_stack_->setCurrentIndex(0); // Default to Overview
    main_content_layout->addWidget(content_stack_);

    // Status bar
    QFrame* status_bar = new QFrame(this);
    status_bar->setStyleSheet("background-color: palette(window); border-top: 1px solid palette(mid);");
    QHBoxLayout* status_layout = new QHBoxLayout(status_bar);
    status_layout->setContentsMargins(15, 8, 15, 8);
    
    QLabel* status_icon = new QLabel("●", this);
    status_icon->setStyleSheet("color: #66cc66; font-size: 10px;");
    status_layout->addWidget(status_icon);
    
    QLabel* status_label = new QLabel("Connected to registry", this);
    status_label->setStyleSheet("color: palette(text-disabled); font-size: 11px;");
    status_layout->addWidget(status_label);
    
    status_layout->addStretch();
    
    QLabel* version_label = new QLabel("v0.1.0", this);
    version_label->setStyleSheet("color: palette(text-disabled); font-size: 10px; font-weight: 500;");
    status_layout->addWidget(version_label);

    main_content_layout->addWidget(status_bar);
    main_layout->addWidget(main_content);
}

void MakekeeperWindow::on_nav_clicked(int index) {
    // Update button states
    nav_overview_->setChecked(index == 0);
    nav_packages_->setChecked(index == 1);
    nav_updates_->setChecked(index == 2);
    nav_registry_->setChecked(index == 3);
    nav_settings_->setChecked(index == 4);
    
    // Switch view
    content_stack_->setCurrentIndex(index);
    
    // Refresh the view
    auto* view = qobject_cast<views::BaseView*>(content_stack_->widget(index));
    if (view) {
        view->refresh();
    }
}

} // namespace gui
} // namespace makekeeper
