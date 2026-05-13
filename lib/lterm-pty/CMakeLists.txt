cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(lterm-pty STATIC
    include/PtyConnection.hpp
    src/PtyConnection.cpp
)

set_target_properties(lterm-pty PROPERTIES
    AUTOMOC ON
)

target_include_directories(lterm-pty
    PUBLIC include
)

target_link_libraries(lterm-pty
    PUBLIC Qt6::Core
    PRIVATE util   # openpty/forkpty
)

target_compile_features(lterm-pty PUBLIC cxx_std_20)
