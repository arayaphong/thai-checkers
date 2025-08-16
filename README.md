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
thai-checkers/
├── src/                 # Source files
│   ├── main.cpp        # Main application entry point
│   ├── Board.cpp       # Game board implementation
│   ├── Explorer.cpp    # Unified piece movement analysis
│   ├── Position.cpp    # Board position utilities
│   └── Legals.cpp      # Legal moves wrapper
├── include/            # Header files
│   ├── Board.h         # Game board interface
│   ├── Explorer.h      # Unified analyzer interface
│   ├── Position.h      # Position utilities
│   ├── Piece.h         # Piece definitions
│   ├── Legals.h        # Legal moves wrapper
│   └── main.h          # Main application interface
├── tests/              # Unit tests
│   ├── BoardTest.cpp
│   ├── ExplorerTest.cpp
│   ├── DameAnalyzerTest.cpp  # Dame-focused tests using unified Explorer
│   ├── PionAnalyzerTest.cpp  # Pion-focused tests using unified Explorer
│   └── PositionTest.cpp
├── build/              # Build output (generated)
├── .vscode/            # VS Code configuration
│   ├── tasks.json      # Build tasks
│   ├── launch.json     # Debug configuration
│   └── c_cpp_properties.json  # IntelliSense configuration
├── .github/
│   └── copilot-instructions.md  # Copilot guidelines
├── backup_coverage_scripts/  # Backup of old coverage scripts
├── coverage.sh         # Unified coverage management script
├── CMakeLists.txt      # Build configuration
└── README.md           # This file
```

## Coverage Analysis

The project includes comprehensive code coverage analysis tools:

### Unified Coverage Script

```bash
# Analyze existing coverage data
./coverage.sh

# Regenerate coverage data and analyze
./coverage.sh --regenerate

# View HTML coverage report in browser
./coverage.sh --view

# Show help
./coverage.sh --help
```

### Coverage Features

- **Line coverage analysis** with detailed reporting
- **HTML reports** with interactive visualization
- **Unified analyzer coverage** for the integrated Explorer class
- **Test execution analysis** with assertion counts
- **Comprehensive game logic validation**

### Current Coverage Status

- **Overall Project**: High line coverage across all components
- **Unified Explorer**: Comprehensive coverage for both piece types
- All unit tests passing with full test suite coverage

## Getting Started

The project implements Thai Checkers game logic with a unified analyzer handling two piece types:

- **Dame (Queens)**: Multi-directional, multi-step movement with complex captures
- **Pion (Pawns)**: Forward-only, single-step movement with simpler captures

## Development Tips

- Use the VS Code tasks for seamless building and running
- The project is configured with C++20 standard
- Debug symbols are enabled for better debugging experience
- IntelliSense is configured for modern C++ features
- Use `./coverage.sh --regenerate` after making changes to update coverage
- View detailed coverage reports with `./coverage.sh --view`
