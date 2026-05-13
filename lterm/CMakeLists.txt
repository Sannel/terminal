cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(KF6 REQUIRED COMPONENTS CoreAddons I18n XmlGui)

add_executable(lterm
    src/main.cpp
    src/MainWindow.hpp
    src/MainWindow.cpp
    src/SettingsDialog.hpp
    src/SettingsDialog.cpp
)

set_target_properties(lterm PROPERTIES AUTOMOC ON)

target_include_directories(lterm PRIVATE src)

target_link_libraries(lterm
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        KF6::CoreAddons
        KF6::I18n
        KF6::XmlGui
        lterm-widget
        lterm-settings
)

target_compile_features(lterm PRIVATE cxx_std_20)

install(TARGETS lterm DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES lterm.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
