cmake_minimum_required(VERSION 3.20)
project(thai-checkers-2 VERSION 1.0.0 LANGUAGES CXX)

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Coverage option
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

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
    src/Explorer.cpp
    src/Options.cpp
    src/Game.cpp
    # Add other source files here
)

# Create the main executable
add_executable(thai_checkers_main
    src/main.cpp
)

# Link the main executable with the library
target_link_libraries(thai_checkers_main thai_checkers_lib)

include(CTest)
if(BUILD_TESTING)
    include(FetchContent)
    # Fetch Catch2 v3
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.3 # stable tag
    )
    FetchContent_MakeAvailable(Catch2)

    # Selector deterministic tests
    add_executable(selector_tests
        src/tests/SelectorTest.cpp)
    target_link_libraries(selector_tests
        PRIVATE
            thai_checkers_lib
            Catch2::Catch2WithMain
    )

    include(Catch)
    catch_discover_tests(selector_tests)
endif()

# Add coverage target if enabled
if(ENABLE_COVERAGE)
    include(Coverage)
    # Coverage target optional when tests are removed
    add_coverage_target()
endif()
