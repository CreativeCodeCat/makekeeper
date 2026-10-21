#include "overview.h"
#include <QVBoxLayout>
#include <QLabel>

namespace makekeeper {
namespace gui {
namespace views {

OverviewView::OverviewView(QWidget* parent) : BaseView(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* title = new QLabel("Overview", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    layout->addWidget(title);
    
    QLabel* content = new QLabel("Overview view is under construction.", this);
    content->setStyleSheet("color: #888888; font-size: 14px;");
    layout->addWidget(content);
    
    layout->addStretch();
}

void OverviewView::refresh() {
    // Refresh overview data
}

} // namespace views
} // namespace gui
} // namespace makekeeper
