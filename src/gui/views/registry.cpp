#include "registry.h"
#include <QVBoxLayout>
#include <QLabel>

namespace makekeeper {
namespace gui {
namespace views {

RegistryView::RegistryView(QWidget* parent) : BaseView(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* title = new QLabel("Registry", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    layout->addWidget(title);
    
    QLabel* content = new QLabel("Registry view is under construction.", this);
    content->setStyleSheet("color: #888888; font-size: 14px;");
    layout->addWidget(content);
    
    layout->addStretch();
}

void RegistryView::refresh() {
    // Refresh registry data
}

} // namespace views
} // namespace gui
} // namespace makekeeper
