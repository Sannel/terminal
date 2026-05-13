cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(lterm-core STATIC
    include/LTerminal.hpp
    include/LTerminalDispatch.hpp
    src/TextBuffer.cpp
    src/LTerminalDispatch.cpp
    src/LTerminal.cpp
)

set_target_properties(lterm-core PROPERTIES
    AUTOMOC ON
)

target_include_directories(lterm-core PUBLIC include)

target_link_libraries(lterm-core
    PUBLIC
        lterm-vt
        lterm-pty
        Qt6::Core
)

target_compile_features(lterm-core PUBLIC cxx_std_20)
target_compile_options(lterm-core PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions)

