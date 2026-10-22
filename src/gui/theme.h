#ifndef MAKEKEEPER_GUI_THEME_H
#define MAKEKEEPER_GUI_THEME_H

#include <QColor>
#include <QPalette>
#include <QString>

class QWidget;
class QAbstractItemView;

namespace makekeeper {
namespace gui {
namespace theme {

// The GUI never sets its own palette or style; everything comes from the user's Qt 6 theme.
// Stylesheets should only use palette(...) roles, and these helpers cover what roles can't.

// Show a widget's text in the theme's secondary (placeholder) colour.
// Works on every Qt 6 version, unlike palette(placeholder-text) in stylesheets
void set_secondary(QWidget* widget);

// Whether the active theme has a dark window background
bool is_dark();

// Status colours that stay readable on the active theme's background
QColor success_color();
QColor warning_color();
QColor error_color();

// Stop a table or list from outlining the current cell. With whole-row selection the highlight
// already shows the current row, and styles such as Union also frame the focused cell inside it
void remove_focus_frame(QAbstractItemView* view);

// A palette colour as #rrggbb, for HTML in rich-text widgets
QString color_name(QPalette::ColorRole role);

} // namespace theme
} // namespace gui
} // namespace makekeeper

#endif // MAKEKEEPER_GUI_THEME_H
