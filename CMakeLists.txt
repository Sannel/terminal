cmake_minimum_required(VERSION 3.21)
project(KTerm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Required so Qt6Qml cmake macros don't error on version check
set(QT_DEFAULT_MAJOR_VERSION 6)

# KDE Extra CMake Modules
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMSetupVersion)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KTERM
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kterm_version.h"
)

# Qt6 — core components required now; Quick/QML added in Phase 6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    DBus
)
qt_standard_project_setup()

# KDE Frameworks 6
find_package(KF6 REQUIRED COMPONENTS
    CoreAddons
    Config
    WidgetsAddons
    WindowSystem
    Notifications
    IconThemes
    XmlGui
    I18n
)

# Optional: Wayland support
find_package(Qt6 COMPONENTS WaylandClient)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_subdirectory(lib/kterm-vt)
add_subdirectory(lib/kterm-pty)
add_subdirectory(lib/kterm-core)
add_subdirectory(lib/kterm-settings)
add_subdirectory(lib/kterm-widget)
add_subdirectory(kterm)
