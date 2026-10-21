#ifndef MAKEKEEPER_GUI_VIEWS_BASE_H
#define MAKEKEEPER_GUI_VIEWS_BASE_H

#include <QWidget>

namespace makekeeper {
namespace gui {
namespace views {

class BaseView : public QWidget {
    Q_OBJECT

public:
    BaseView(QWidget* parent = nullptr);
    virtual ~BaseView();
    virtual void refresh() = 0;
};

} // namespace views
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_VIEWS_BASE_H
