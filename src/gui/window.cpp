#include "window.h"
#include "../config.h"
#include "theme.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QShortcut>
#include <QKeySequence>

namespace makekeeper {
namespace gui {

MakekeeperWindow::MakekeeperWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QString("MakeKeeper | Package Manager %1").arg(config::VERSION));
    setFixedSize(1100, 650);

    // No palette or style is set here: the window follows the user's Qt 6 theme
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

void MakekeeperWindow::setup_ui() {
    QWidget* central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    QHBoxLayout* main_layout = new QHBoxLayout(central_widget);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // Sidebar
    QWidget* sidebar = new QWidget(this);
    sidebar->setFixedWidth(140);
    sidebar->setObjectName("sidebar");
    sidebar->setAttribute(Qt::WA_StyledBackground, true);
    sidebar->setStyleSheet("QWidget#sidebar { background-color: palette(window); border-right: 1px solid palette(mid); }");

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

    // The checked highlight already marks the current page; without this, styles such as Union
    // also draw a focus outline inside it after a click. Ctrl+1 to Ctrl+5 switch pages instead
    QPushButton* nav_buttons[] = {nav_overview_, nav_packages_, nav_updates_, nav_registry_, nav_settings_};
    for (int i = 0; i < 5; i++) {
        nav_buttons[i]->setFocusPolicy(Qt::NoFocus);
        nav_buttons[i]->setToolTip(QString("%1 (Ctrl+%2)").arg(nav_buttons[i]->text()).arg(i + 1));
        QShortcut* shortcut = new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)), this);
        QObject::connect(shortcut, &QShortcut::activated, this, [this, i]() { on_nav_clicked(i); });
    }

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
    status_bar->setObjectName("statusBar");
    status_bar->setStyleSheet("QFrame#statusBar { background-color: palette(window); border-top: 1px solid palette(mid); }");
    QHBoxLayout* status_layout = new QHBoxLayout(status_bar);
    status_layout->setContentsMargins(15, 8, 15, 8);
    
    QLabel* status_icon = new QLabel("●", this);
    status_icon->setStyleSheet(QString("color: %1; font-size: 10px;").arg(theme::success_color().name()));
    status_layout->addWidget(status_icon);
    
    QLabel* status_label = new QLabel("Connected to registry", this);
    status_label->setStyleSheet("font-size: 11px;");
    theme::set_secondary(status_label);
    status_layout->addWidget(status_label);
    
    status_layout->addStretch();
    
    QLabel* version_label = new QLabel(config::VERSION, this);
    version_label->setStyleSheet("font-size: 10px; font-weight: 500;");
    theme::set_secondary(version_label);
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
