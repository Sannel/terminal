add_library(lterm-widget STATIC
    include/TerminalWidget.hpp
    src/TerminalWidget.cpp
)

set_target_properties(lterm-widget PROPERTIES
    AUTOMOC ON
)

target_include_directories(lterm-widget
    PUBLIC  include
    PRIVATE src
)

target_link_libraries(lterm-widget
    PUBLIC
        lterm-core
        lterm-settings
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_compile_features(lterm-widget PUBLIC cxx_std_20)
