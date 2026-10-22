#ifndef MAKEKEEPER_CONFIG_H
#define MAKEKEEPER_CONFIG_H

#include <string>

namespace makekeeper {
namespace config {

// Paths
inline constexpr const char* REGISTRY_DIR = "/var/lib/makekeeper";
inline constexpr const char* CACHE_DIR = "/var/lib/makekeeper/cache";
inline constexpr const char* CACHE_FILE = "/var/lib/makekeeper/cache/packages.json";
inline constexpr const char* REGISTRY_FILE = "/var/lib/makekeeper/registry.json";
inline constexpr const char* APP_ROOT = "/opt/makekeeper/apps";
inline constexpr const char* BIN_DIR = "/usr/local/bin";
inline constexpr const char* MAN_DIR = "/usr/share/man/man1";
inline constexpr const char* DESKTOP_DIR = "/usr/local/share/applications";
inline constexpr const char* ICON_DIR = "/usr/local/share/icons/hicolor/128x128/apps";
inline constexpr const char* ICON_SVG_DIR = "/usr/local/share/icons/hicolor/scalable/apps";

// Package catalog
inline constexpr const char* PACKAGES_URL = "https://creativecodecat.github.io/makekeeper/packages.json";

// Version
inline constexpr const char* VERSION = "1.0.0";

} // namespace config
} // namespace makekeeper

#endif // MAKEKEEPER_CONFIG_H
