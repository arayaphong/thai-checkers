# Thai Checkers 2

A modern C++ implementation of Thai Checkers using C++20 features.

## Features

- Modern C++20 codebase
- CMake build system
- Debug support with GDB
- VS Code integration with IntelliSense

## Requirements

- CMake 3.20 or higher
- GCC 11+ or Clang 13+ (with C++20 support)
- GDB (for debugging)

## Building the Project

### Using VS Code (Recommended)

1. Open the project in VS Code
2. Press `Ctrl+Shift+P` and run "Tasks: Run Task"
3. Select "CMake Build" to build the project
4. To run: Select "Run Application" task

### Using Command Line

```bash
# Configure the build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# Build the project
cmake --build build --config Debug

# Run the executable
./build/ThaiCheckers2
```

## Debugging

1. Set breakpoints in your code
2. Press `F5` or go to Run and Debug view
3. Select "Debug Thai Checkers 2" configuration
4. The debugger will build and launch the application

## Project Structure

```
thai-checkers2/
├── src/                 # Source files
│   └── main.cpp        # Main application entry point
├── include/            # Header files
├── build/              # Build output (generated)
├── .vscode/            # VS Code configuration
│   ├── tasks.json      # Build tasks
│   ├── launch.json     # Debug configuration
│   └── c_cpp_properties.json  # IntelliSense configuration
├── .github/
│   └── copilot-instructions.md  # Copilot guidelines
├── CMakeLists.txt      # Build configuration
└── README.md           # This file
```

## Getting Started

The project includes a simple "Hello World" application that demonstrates C++20 features. You can start building your Thai Checkers game from there.

## Development Tips

- Use the VS Code tasks for seamless building and running
- The project is configured with C++20 standard
- Debug symbols are enabled for better debugging experience
- IntelliSense is configured for modern C++ features
