#ifndef MAKEKEEPER_GUI_VIEWS_REGISTRY_H
#define MAKEKEEPER_GUI_VIEWS_REGISTRY_H

#include "base.h"

namespace makekeeper {
namespace gui {
namespace views {

class RegistryView : public BaseView {
    Q_OBJECT

public:
    RegistryView(QWidget* parent = nullptr);
    void refresh() override;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_REGISTRY_H
