cmake_minimum_required(VERSION 3.16)
project(makekeeper VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)
pkg_check_modules(CURL REQUIRED libcurl)

# nlohmann_json - header-only, check if available via pkg-config or include path
pkg_check_modules(NLOHMANN_JSON nlohmann_json)
if(NOT NLOHMANN_JSON_FOUND)
    # Use header-only approach - just include directory
    set(NLOHMANN_JSON_INCLUDE_DIR "/usr/include/nlohmann")
    if(EXISTS "${NLOHMANN_JSON_INCLUDE_DIR}")
        include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
    else()
        message(WARNING "nlohmann_json not found. Installing via package manager is recommended.")
    endif()
endif()

# Include directories
include_directories(
    ${LIBGIT2_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6Network_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/cli.cpp
    src/config.cpp
    src/registry.cpp
    src/utils.cpp
    src/commands/install.cpp
    src/commands/uninstall.cpp
    src/commands/update.cpp
    src/commands/list.cpp
    src/commands/search.cpp
    src/commands/sync.cpp
    src/commands/fetch.cpp
    src/commands/gui.cpp
    src/commands/info.cpp
    src/gui/gui.cpp
    src/gui/window.cpp
    src/gui/theme.cpp
    src/gui/views/base.cpp
    src/gui/views/overview.cpp
    src/gui/views/packages.cpp
    src/gui/views/updates.cpp
    src/gui/views/registry.cpp
    src/gui/views/settings.cpp
)

set(HEADERS
    src/cli.h
    src/config.h
    src/registry.h
    src/utils.h
    src/commands/install.h
    src/commands/uninstall.h
    src/commands/update.h
    src/commands/list.h
    src/commands/search.h
    src/commands/sync.h
    src/commands/fetch.h
    src/gui/gui.h
    src/gui/window.h
    src/gui/theme.h
)

# Create executable
add_executable(makekeeper ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(makekeeper
    ${LIBGIT2_LIBRARIES}
    ${CURL_LIBRARIES}
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Privileged helper, run through pkexec. It only needs the standard library and
# nlohmann_json, and links libstdc++ statically so it still runs when copied out of the AppImage.
add_executable(makekeeper-helper src/helper/main.cpp)
target_link_options(makekeeper-helper PRIVATE -static-libstdc++ -static-libgcc)
set_target_properties(makekeeper-helper PROPERTIES AUTOMOC OFF AUTORCC OFF AUTOUIC OFF)

# polkit policy, generated for the install location of the helper
set(MAKEKEEPER_HELPER_INSTALL_PATH "/usr/local/lib/makekeeper/makekeeper-helper")
set(HELPER_PATH "${MAKEKEEPER_HELPER_INSTALL_PATH}")
configure_file(makekeeper.policy.in ${CMAKE_BINARY_DIR}/io.github.creativecodecat.makekeeper.policy @ONLY)

# Installation
install(TARGETS makekeeper DESTINATION /usr/local/bin)
install(TARGETS makekeeper-helper DESTINATION /usr/local/lib/makekeeper)
install(FILES ${CMAKE_BINARY_DIR}/io.github.creativecodecat.makekeeper.policy DESTINATION /usr/share/polkit-1/actions)
install(FILES makekeeper.1 DESTINATION /usr/local/share/man/man1)
install(FILES makekeeper.desktop DESTINATION /usr/local/share/applications)
install(FILES makekeeper.svg DESTINATION /usr/local/share/icons/hicolor/scalable/apps)
