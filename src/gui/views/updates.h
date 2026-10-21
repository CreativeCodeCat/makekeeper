#ifndef MAKEKEEPER_GUI_VIEWS_UPDATES_H
#define MAKEKEEPER_GUI_VIEWS_UPDATES_H

#include "base.h"

namespace makekeeper {
namespace gui {
namespace views {

class UpdatesView : public BaseView {
    Q_OBJECT

public:
    UpdatesView(QWidget* parent = nullptr);
    void refresh() override;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_UPDATES_H
