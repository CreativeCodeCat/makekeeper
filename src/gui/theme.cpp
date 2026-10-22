#include "theme.h"
#include <QApplication>
#include <QWidget>
#include <QAbstractItemView>
#include <QStyledItemDelegate>

namespace makekeeper {
namespace gui {
namespace theme {

namespace {

class NoFocusFrameDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem item_option(option);
        item_option.state &= ~QStyle::State_HasFocus;
        QStyledItemDelegate::paint(painter, item_option, index);
    }
};

} // namespace

void set_secondary(QWidget* widget) {
    widget->setForegroundRole(QPalette::PlaceholderText);
}

bool is_dark() {
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
}

QColor success_color() {
    return is_dark() ? QColor("#66cc66") : QColor("#2e7d32");
}

QColor warning_color() {
    return is_dark() ? QColor("#d9a066") : QColor("#a85d00");
}

QColor error_color() {
    return is_dark() ? QColor("#e06c6c") : QColor("#c62828");
}

void remove_focus_frame(QAbstractItemView* view) {
    view->setItemDelegate(new NoFocusFrameDelegate(view));
}

QString color_name(QPalette::ColorRole role) {
    return QApplication::palette().color(role).name();
}

} // namespace theme
} // namespace gui
} // namespace makekeeper
