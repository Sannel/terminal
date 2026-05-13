cmake_minimum_required(VERSION 3.21)

add_library(kterm-vt STATIC
    src/stateMachine.cpp
)

target_include_directories(kterm-vt
    PUBLIC  include
)

target_compile_features(kterm-vt PUBLIC cxx_std_20)

target_compile_options(kterm-vt PRIVATE
    -Wall -Wextra -Wno-unused-parameter -fexceptions
)
