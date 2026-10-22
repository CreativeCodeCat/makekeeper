#ifndef MAKEKEEPER_COLORS_H
#define MAKEKEEPER_COLORS_H

#include <cstdlib>
#include <unistd.h>

namespace makekeeper {
namespace colors {

// Colour only when writing to a terminal, so piped output stays plain,
// and never when NO_COLOR is set (https://no-color.org)
inline bool use_color() {
    const char* no_color = std::getenv("NO_COLOR");
    return isatty(STDOUT_FILENO) && !(no_color && *no_color);
}

inline const bool enabled = use_color();

// ANSI color codes, matching the GUI's palette
inline const char* const reset = enabled ? "\033[0m" : "";
inline const char* const green = enabled ? "\033[38;2;102;204;102m" : "";   // #66cc66
inline const char* const cyan = enabled ? "\033[38;2;61;174;233m" : "";     // #3DAEE9
inline const char* const gray = enabled ? "\033[38;2;127;133;139m" : "";    // #7F858B
inline const char* const yellow = enabled ? "\033[38;2;238;201;85m" : "";   // #EEC955

} // namespace colors
} // namespace makekeeper

#endif // MAKEKEEPER_COLORS_H
