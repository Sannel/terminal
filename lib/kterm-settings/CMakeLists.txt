cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(kterm-settings STATIC
    include/KTermSettings.hpp
    include/ColorScheme.hpp
    include/Profile.hpp
    src/ColorScheme.cpp
    src/Profile.cpp
    src/KTermSettings.cpp
)

set_target_properties(kterm-settings PROPERTIES
    AUTOMOC ON
)

target_include_directories(kterm-settings PUBLIC include)

target_link_libraries(kterm-settings
    PUBLIC Qt6::Core Qt6::Gui
)

target_compile_features(kterm-settings PUBLIC cxx_std_20)

