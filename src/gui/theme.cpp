#include "theme.h"
#include <QApplication>
#include <QWidget>
#include <QAbstractItemView>
#include <QStyledItemDelegate>
#include <algorithm>
#include <cmath>

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

// WCAG relative luminance and contrast ratio
static double luminance(const QColor& color) {
    auto channel = [](double c) { return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); };
    return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF()) + 0.0722 * channel(color.blueF());
}

static double contrast(const QColor& a, const QColor& b) {
    double la = luminance(a);
    double lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

QColor link_color() {
    QPalette palette = QApplication::palette();
    QColor base = palette.color(QPalette::Base);
    QColor link = palette.color(QPalette::Link);

    // Lighten on dark backgrounds, darken on light ones, keeping the theme's hue
    bool dark_base = luminance(base) < 0.2;
    for (int i = 0; i < 6 && contrast(link, base) < 4.5; i++) {
        link = dark_base ? link.lighter(130) : link.darker(130);
        if (dark_base && link.value() == 255) {
            link.setHsv(link.hue(), std::max(0, link.saturation() - 40), 255);
        }
    }
    return link;
}

void remove_focus_frame(QAbstractItemView* view) {
    view->setItemDelegate(new NoFocusFrameDelegate(view));
}

QString primary_button_style() {
    return "QPushButton { background-color: palette(highlight); color: palette(highlighted-text); border: 1px solid palette(highlight); border-radius: 4px; padding: 8px 18px; font-size: 13px; font-weight: 500; }"
           "QPushButton:hover { border-color: palette(highlighted-text); }"
           "QPushButton:disabled { background-color: palette(button); color: palette(mid); border-color: palette(mid); }";
}

QString secondary_button_style() {
    return "QPushButton { background-color: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 4px; padding: 8px 18px; font-size: 13px; font-weight: 500; }"
           "QPushButton:hover { border-color: palette(highlight); }"
           "QPushButton:disabled { color: palette(mid); }";
}

QString danger_button_style() {
    return QString(
               "QPushButton { background-color: %1; color: white; border: 1px solid %1; border-radius: 4px; padding: 8px 18px; font-size: 13px; font-weight: 500; }"
               "QPushButton:hover { border-color: palette(window-text); }"
               "QPushButton:disabled { background-color: palette(button); color: palette(mid); border-color: palette(mid); }")
        .arg(error_color().name());
}

QString color_name(QPalette::ColorRole role) {
    return QApplication::palette().color(role).name();
}

} // namespace theme
} // namespace gui
} // namespace makekeeper
