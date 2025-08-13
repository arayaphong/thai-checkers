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

# Optional CPU/Link optimizations
option(ENABLE_NATIVE_OPTIMIZATIONS "Enable -march/-mtune=native for Release builds" OFF)
set(AMD_ZEN_ARCH "" CACHE STRING "Explicit AMD Zen micro-arch to target: znver2, znver3, znver4")
option(ENABLE_LTO "Enable Link Time Optimization (IPO) in Release" OFF)

# Optional: Enable OpenMP if available
find_package(OpenMP QUIET)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Link Time Optimization (IPO) (global project setting)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
        if(ipo_supported)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
            message(STATUS "IPO/LTO enabled for Release builds")
        else()
            message(WARNING "IPO/LTO requested but not supported: ${ipo_error}")
        endif()
    endif()
endif()

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
    src/Legals.cpp
    src/Game.cpp
    src/Traversal.cpp
    # Add other source files here
)

# Create the main executable
add_executable(thai_checkers_main
    src/main.cpp
)

# Link the main executable with the library
target_link_libraries(thai_checkers_main PRIVATE thai_checkers_lib)

if(OpenMP_CXX_FOUND)
    target_link_libraries(thai_checkers_main PRIVATE OpenMP::OpenMP_CXX)
    # Also apply OpenMP to the library, since it contains parallel code (Traversal.cpp)
    target_link_libraries(thai_checkers_lib PRIVATE OpenMP::OpenMP_CXX)
    message(STATUS "OpenMP found: enabling parallel build flags")
else()
    message(STATUS "OpenMP not found: building without OpenMP support")
endif()

# Apply arch-specific flags for project targets only (after targets are defined)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(AMD_ZEN_ARCH)
        target_compile_options(thai_checkers_lib PRIVATE
            $<$<CONFIG:Release>:-march=${AMD_ZEN_ARCH}>
            $<$<CONFIG:Release>:-mtune=${AMD_ZEN_ARCH}>
        )
        target_compile_options(thai_checkers_main PRIVATE
            $<$<CONFIG:Release>:-march=${AMD_ZEN_ARCH}>
            $<$<CONFIG:Release>:-mtune=${AMD_ZEN_ARCH}>
        )
    elseif(ENABLE_NATIVE_OPTIMIZATIONS)
        target_compile_options(thai_checkers_lib PRIVATE
            $<$<CONFIG:Release>:-march=native>
            $<$<CONFIG:Release>:-mtune=native>
        )
        target_compile_options(thai_checkers_main PRIVATE
            $<$<CONFIG:Release>:-march=native>
            $<$<CONFIG:Release>:-mtune=native>
        )
    endif()
endif()

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
