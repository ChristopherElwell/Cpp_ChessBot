cmake_minimum_required(VERSION 3.14)

project(ElwellBot)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(ElwellBot ${SOURCES})
