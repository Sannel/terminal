cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(kterm-core STATIC
    include/KTerminal.hpp
    include/KTerminalDispatch.hpp
    src/TextBuffer.cpp
    src/KTerminalDispatch.cpp
    src/KTerminal.cpp
)

set_target_properties(kterm-core PROPERTIES
    AUTOMOC ON
)

target_include_directories(kterm-core PUBLIC include)

target_link_libraries(kterm-core
    PUBLIC
        kterm-vt
        kterm-pty
        Qt6::Core
)

target_compile_features(kterm-core PUBLIC cxx_std_20)
target_compile_options(kterm-core PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions)

