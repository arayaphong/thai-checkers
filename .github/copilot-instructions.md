<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# Thai Checkers 2 - C++ Project Instructions

This is a C++ project that uses:
- **C++20 standard** with modern features
- **CMake** for build system management
- **Debug support** with GDB integration
- **VS Code** tasks for building and running

## Code Style Guidelines
- Use modern C++20 features when appropriate
- Follow RAII principles
- Use smart pointers instead of raw pointers
- Prefer `auto` for type deduction where it improves readability
- Use `const` and `constexpr` where applicable
- Follow snake_case for variables and functions, PascalCase for classes

## Project Structure
- `src/` - Source files (.cpp)
- `include/` - Header files (.h, .hpp)
- `build/` - Build output directory (generated)
- `.vscode/` - VS Code configuration files

## Build and Debug
- Use "CMake Build" task (Ctrl+Shift+P -> Tasks: Run Task)
- Use "Debug Thai Checkers 2" configuration for debugging
- The project is configured for debugging with GDB

## Dependencies
- CMake 3.20 or higher
- GCC/Clang with C++20 support
- GDB for debugging
