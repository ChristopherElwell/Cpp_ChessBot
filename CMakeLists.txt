cmake_minimum_required(VERSION 3.14)
project(ElwellBot
    VERSION 1.1
    LANGUAGES CXX
)

# Set output directories based on build type
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)

add_executable(ElwellBot)

# Set C++ standard as a target property
target_compile_features(
    ElwellBot 
        PRIVATE 
        cxx_std_23)

# Include directories (if you have headers in include/)
target_sources(ElwellBot PRIVATE 
    src/bitboard.cpp
    src/engine.cpp
    src/eval.cpp
    src/main.cpp    
    src/move_gen.cpp
    src/move.cpp
    src/search.cpp
    src/testing.cpp
)

target_compile_options(ElwellBot PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
    $<$<CONFIG:Release>:-O3>
)