cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(lterm-settings STATIC
    include/LTermSettings.hpp
    include/ColorScheme.hpp
    include/Profile.hpp
    src/ColorScheme.cpp
    src/Profile.cpp
    src/LTermSettings.cpp
)

set_target_properties(lterm-settings PROPERTIES
    AUTOMOC ON
)

target_include_directories(lterm-settings PUBLIC include)

target_link_libraries(lterm-settings
    PUBLIC Qt6::Core Qt6::Gui
)

target_compile_features(lterm-settings PUBLIC cxx_std_20)

