add_library(kterm-widget STATIC
    include/TerminalWidget.hpp
    src/TerminalWidget.cpp
)

set_target_properties(kterm-widget PROPERTIES
    AUTOMOC ON
)

target_include_directories(kterm-widget
    PUBLIC  include
    PRIVATE src
)

target_link_libraries(kterm-widget
    PUBLIC
        kterm-core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_compile_features(kterm-widget PUBLIC cxx_std_20)
