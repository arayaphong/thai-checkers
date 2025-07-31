cmake_minimum_required(VERSION 3.20)
project(thai-checkers-2 VERSION 1.0.0 LANGUAGES CXX)

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Coverage option
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

# Find Catch2 for tests
find_package(Catch2 3 REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Setup coverage if enabled
if(ENABLE_COVERAGE)
    include(Coverage)
    setup_coverage()
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Create a library from the source files
add_library(thai_checkers_lib STATIC
    src/Board.cpp
    src/Position.cpp
    src/Move.cpp
    src/DameAnalyzer.cpp
    src/PionAnalyzer.cpp
    src/Options.cpp
    # Add other source files here
)

# Create the main executable
add_executable(thai_checkers_main
    src/main.cpp
)

# Link the main executable with the library
target_link_libraries(thai_checkers_main thai_checkers_lib)

# Enable testing
enable_testing()

# Add test subdirectory (corrected directory name)
add_subdirectory(tests)

# Add coverage target if enabled
if(ENABLE_COVERAGE)
    include(Coverage)
    add_coverage_target()
endif()
