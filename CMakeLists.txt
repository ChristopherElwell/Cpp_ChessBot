cmake_minimum_required(VERSION 3.10.0)
project(ElwellBot)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler path: ${CMAKE_CXX_COMPILER}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(ElwellBot ${SRC_FILES})

