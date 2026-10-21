#ifndef MAKEKEEPER_GUI_VIEWS_OVERVIEW_H
#define MAKEKEEPER_GUI_VIEWS_OVERVIEW_H

#include "base.h"
#include <QLabel>

namespace makekeeper {
namespace gui {
namespace views {

class OverviewView : public BaseView {
    Q_OBJECT

public:
    OverviewView(QWidget* parent = nullptr);
    void refresh() override;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_OVERVIEW_H
