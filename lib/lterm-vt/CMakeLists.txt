cmake_minimum_required(VERSION 3.21)

add_library(lterm-vt STATIC
    src/stateMachine.cpp
)

target_include_directories(lterm-vt
    PUBLIC  include
)

target_compile_features(lterm-vt PUBLIC cxx_std_20)

target_compile_options(lterm-vt PRIVATE
    -Wall -Wextra -Wno-unused-parameter -fexceptions
)
