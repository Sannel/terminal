cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(KF6 REQUIRED COMPONENTS CoreAddons I18n XmlGui)

add_executable(kterm
    src/main.cpp
    src/MainWindow.hpp
    src/MainWindow.cpp
)

set_target_properties(kterm PROPERTIES AUTOMOC ON)

target_include_directories(kterm PRIVATE src)

target_link_libraries(kterm
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        KF6::CoreAddons
        KF6::I18n
        KF6::XmlGui
        kterm-widget
        kterm-settings
)

target_compile_features(kterm PRIVATE cxx_std_20)

install(TARGETS kterm DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES kterm.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
