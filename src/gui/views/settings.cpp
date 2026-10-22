#include "settings.h"
#include "../../config.h"
#include "../../utils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QTabWidget>
#include <QWidget>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <Qt>
#include <QObject>

namespace makekeeper {
namespace gui {
namespace views {

SettingsView::SettingsView(QWidget* parent) : BaseView(parent) {
    setup_ui();
    load_settings();
}

void SettingsView::setup_ui() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Create tab widget
    QTabWidget* tab_widget = new QTabWidget(this);
    tab_widget->setStyleSheet(
        "QTabWidget::pane { border: none; background-color: transparent; }"
        "QTabBar::tab { background-color: transparent; color: palette(text-disabled); padding: 10px 20px; margin-right: 2px; font-size: 13px; border: none; }"
        "QTabBar::tab:selected { color: palette(highlight); border-bottom: 2px solid palette(highlight); }"
        "QTabBar::tab:hover { color: palette(text); }"
    );

    // Paths tab
    QWidget* paths_tab = new QWidget();
    QVBoxLayout* paths_layout = new QVBoxLayout(paths_tab);
    paths_layout->setContentsMargins(0, 10, 0, 0);
    paths_layout->setSpacing(15);

    QLabel* cache_label = new QLabel("Cache Directory:", paths_tab);
    cache_label->setStyleSheet("color: palette(text); font-size: 13px;");
    paths_layout->addWidget(cache_label);

    QLineEdit* cache_edit = new QLineEdit(QString::fromStdString(config::CACHE_DIR), paths_tab);
    cache_edit->setReadOnly(true);
    cache_edit->setStyleSheet("QLineEdit { background-color: palette(base); color: palette(text); border: 1px solid #444444; padding: 8px 10px; font-size: 13px; }");
    paths_layout->addWidget(cache_edit);

    QLabel* registry_label = new QLabel("Registry File:", paths_tab);
    registry_label->setStyleSheet("color: palette(text); font-size: 13px;");
    paths_layout->addWidget(registry_label);

    QLineEdit* registry_edit = new QLineEdit(QString::fromStdString(config::REGISTRY_FILE), paths_tab);
    registry_edit->setReadOnly(true);
    registry_edit->setStyleSheet("QLineEdit { background-color: palette(base); color: palette(text); border: 1px solid #444444; padding: 8px 10px; font-size: 13px; }");
    paths_layout->addWidget(registry_edit);

    QLabel* install_label = new QLabel("Install Directory:", paths_tab);
    install_label->setStyleSheet("color: palette(text); font-size: 13px;");
    paths_layout->addWidget(install_label);

    QLineEdit* install_edit = new QLineEdit("/usr/local", paths_tab);
    install_edit->setReadOnly(true);
    install_edit->setStyleSheet("QLineEdit { background-color: palette(base); color: palette(text); border: 1px solid #444444; padding: 8px 10px; font-size: 13px; }");
    paths_layout->addWidget(install_edit);

    paths_layout->addStretch();
    tab_widget->addTab(paths_tab, "Paths");

    // Preferences tab
    QWidget* prefs_tab = new QWidget();
    QVBoxLayout* prefs_layout = new QVBoxLayout(prefs_tab);
    prefs_layout->setContentsMargins(0, 10, 0, 0);
    prefs_layout->setSpacing(12);

    auto_sync_ = new QCheckBox("Auto-sync catalog on startup", prefs_tab);
    auto_sync_->setStyleSheet("QCheckBox { color: palette(text); font-size: 13px; spacing: 8px; }");
    QObject::connect(auto_sync_, &QCheckBox::toggled, this, [this]() { save_settings(); });
    prefs_layout->addWidget(auto_sync_);

    notifications_ = new QCheckBox("Show desktop notifications", prefs_tab);
    notifications_->setStyleSheet("QCheckBox { color: palette(text); font-size: 13px; spacing: 8px; }");
    QObject::connect(notifications_, &QCheckBox::toggled, this, [this]() { save_settings(); });
    prefs_layout->addWidget(notifications_);

    confirm_install_ = new QCheckBox("Confirm before installing packages", prefs_tab);
    confirm_install_->setStyleSheet("QCheckBox { color: palette(text); font-size: 13px; spacing: 8px; }");
    QObject::connect(confirm_install_, &QCheckBox::toggled, this, [this]() { save_settings(); });
    prefs_layout->addWidget(confirm_install_);

    prefs_layout->addStretch();
    tab_widget->addTab(prefs_tab, "Preferences");

    // About tab
    QWidget* about_tab = new QWidget();
    QVBoxLayout* about_layout = new QVBoxLayout(about_tab);
    about_layout->setContentsMargins(0, 10, 0, 0);
    about_layout->setSpacing(10);

    QLabel* version_label = new QLabel(QString("MakeKeeper %1").arg(QString::fromStdString(config::VERSION)), about_tab);
    version_label->setStyleSheet("color: palette(highlight); font-size: 20px; font-weight: bold; margin-bottom: 5px;");
    about_layout->addWidget(version_label);

    QLabel* desc_label = new QLabel("Safer installer for Makefile-based Linux applications", about_tab);
    desc_label->setStyleSheet("color: palette(text-disabled); font-size: 13px;");
    about_layout->addWidget(desc_label);

    about_layout->addStretch();
    tab_widget->addTab(about_tab, "About");

    layout->addWidget(tab_widget);
}

void SettingsView::refresh() {
    // Refresh settings data
}

void SettingsView::load_settings() {
    QString config_path = QString::fromStdString(utils::user_config_dir()) + "/settings.conf";
    QSettings settings(config_path, QSettings::IniFormat);
    
    // Block signals to prevent triggering save during load
    auto_sync_->blockSignals(true);
    notifications_->blockSignals(true);
    confirm_install_->blockSignals(true);
    
    auto_sync_->setChecked(settings.value("auto_sync", false).toBool());
    notifications_->setChecked(settings.value("notifications", true).toBool());
    confirm_install_->setChecked(settings.value("confirm_install", true).toBool());
    
    // Unblock signals
    auto_sync_->blockSignals(false);
    notifications_->blockSignals(false);
    confirm_install_->blockSignals(false);
}

void SettingsView::save_settings() {
    QString config_dir = QString::fromStdString(utils::user_config_dir());
    QString parent_dir = QFileInfo(config_dir).path();
    bool parent_existed = QDir(parent_dir).exists();
    QDir().mkpath(config_dir);
    
    QString config_path = config_dir + "/settings.conf";
    QSettings settings(config_path, QSettings::IniFormat);
    
    settings.setValue("auto_sync", auto_sync_->isChecked());
    settings.setValue("notifications", notifications_->isChecked());
    settings.setValue("confirm_install", confirm_install_->isChecked());
    
    settings.sync();

    // When running as root via sudo/pkexec, keep these files owned by the real user
    if (!parent_existed) {
        utils::chown_to_invoking_user(parent_dir.toStdString());
    }
    utils::chown_to_invoking_user(config_dir.toStdString());
    utils::chown_to_invoking_user(config_path.toStdString());
}

} // namespace views
} // namespace gui
} // namespace makekeeper
