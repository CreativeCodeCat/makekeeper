#include "gui.h"
#include "../config.h"
#include "../registry.h"
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSplitter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <iostream>

namespace makekeeper {
namespace commands {

class MakekeeperWindow : public QMainWindow {
public:
    MakekeeperWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle(QString("MakeKeeper v%1 | Package Manager").arg(config::VERSION));
        setFixedSize(1200, 800);

        setup_ui();
        load_packages();
        load_installed();
    }

private:
    void setup_ui() {
        QWidget* central_widget = new QWidget(this);
        setCentralWidget(central_widget);

        QVBoxLayout* main_layout = new QVBoxLayout(central_widget);

        // Top bar with title and search
        QHBoxLayout* top_layout = new QHBoxLayout();
        QLabel* title_label = new QLabel(QString("MakeKeeper v%1").arg(config::VERSION), this);
        title_label->setStyleSheet("font-size: 18px; font-weight: bold;");
        search_edit_ = new QLineEdit(this);
        search_edit_->setPlaceholderText("Search packages...");
        search_edit_->setMaximumWidth(400);
        QObject::connect(search_edit_, &QLineEdit::textChanged, [this](const QString& text) {
            on_search_changed(text);
        });

        top_layout->addWidget(title_label);
        top_layout->addStretch();
        top_layout->addWidget(search_edit_);

        // Main content with splitter
        QSplitter* splitter = new QSplitter(Qt::Vertical, this);

        // Package list
        package_list_ = new QListWidget(this);
        QObject::connect(package_list_, &QListWidget::itemClicked, [this](QListWidgetItem* item) {
            on_package_selected(item);
        });

        // Details panel
        details_panel_ = new QTextEdit(this);
        details_panel_->setReadOnly(true);
        details_panel_->setMaximumHeight(350);

        splitter->addWidget(package_list_);
        splitter->addWidget(details_panel_);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 0);

        // Status bar
        status_label_ = new QLabel("Connected to registry", this);

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

        for (const auto& pkg : packages) {
            QJsonObject obj = pkg.toObject();
            QString name = obj["name"].toString();
            QString version = obj.contains("latest_version") ? obj["latest_version"].toString() : "latest";
            QString description = obj["description"].toString();

            QString item_text = QString("%1 v%2 - %3").arg(name, version, description);
            package_list_->addItem(item_text);
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
        for (int i = 0; i < package_list_->count(); ++i) {
            QListWidgetItem* item = package_list_->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    }

    void on_package_selected(QListWidgetItem* item) {
        QString text = item->text();
        int v_pos = text.indexOf(" v");
        int dash_pos = text.indexOf(" - ");

        if (v_pos >= 0 && dash_pos >= 0) {
            QString name = text.left(v_pos);
            QString version = text.mid(v_pos + 2, dash_pos - v_pos - 2);
            QString description = text.mid(dash_pos + 3);

            QString details = QString(
                "<h2>%1 v%2</h2>"
                "<p>%3</p>"
                "<p><b>Status:</b> %4</p>"
                "<p><b>Actions:</b></p>"
                "<button>Install</button> <button>Update</button> <button>Uninstall</button>"
            ).arg(name, version, description, installed_names_.contains(name) ? "Installed" : "Not installed");

            details_panel_->setHtml(details);
        }
    }

private:
    QLineEdit* search_edit_;
    QListWidget* package_list_;
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
