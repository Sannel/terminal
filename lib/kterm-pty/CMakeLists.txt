cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(kterm-pty STATIC
    include/PtyConnection.hpp
    src/PtyConnection.cpp
)

set_target_properties(kterm-pty PROPERTIES
    AUTOMOC ON
)

target_include_directories(kterm-pty
    PUBLIC include
)

target_link_libraries(kterm-pty
    PUBLIC Qt6::Core
    PRIVATE util   # openpty/forkpty
)

target_compile_features(kterm-pty PUBLIC cxx_std_20)
