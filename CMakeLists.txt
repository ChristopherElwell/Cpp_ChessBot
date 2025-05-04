cmake_minimum_required(VERSION 3.10)
project(ElwellBot VERSION 0.1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# For Windows with MinGW
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

# Debug vs Release configuration
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Set compiler flags based on build type (GCC specific)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Add all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Add executable
add_executable(ElwellBot ${SOURCES})

# Add any needed libraries
# target_link_libraries(ElwellBot PRIVATE lib1 lib2)

# Install instructions
install(TARGETS ElwellBot DESTINATION bin)