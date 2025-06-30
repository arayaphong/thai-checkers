cmake_minimum_required(VERSION 3.20)
project(ThaiCheckers2)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable debug symbols
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Add compile flags for better debugging experience
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Include directories
include_directories(include)

# Add executable
add_executable(${PROJECT_NAME}
    src/main.cpp
)

# Optional: Add subdirectories if you have more source files
# add_subdirectory(src)
