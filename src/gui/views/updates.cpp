#include "updates.h"
#include "../theme.h"
#include <QVBoxLayout>
#include <QLabel>

namespace makekeeper {
namespace gui {
namespace views {

UpdatesView::UpdatesView(QWidget* parent) : BaseView(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* title = new QLabel("Updates", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(window-text);");
    layout->addWidget(title);
    
    QLabel* content = new QLabel("Updates view is under construction.", this);
    content->setStyleSheet("font-size: 14px;");
    theme::set_secondary(content);
    layout->addWidget(content);
    
    layout->addStretch();
}

void UpdatesView::refresh() {
    // Refresh updates data
}

} // namespace views
} // namespace gui
} // namespace makekeeper
