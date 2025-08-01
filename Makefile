.PHONY: all clean test run

# Default target: build the project (all)
all:
	cmake -S . -B build
	cmake --build build --config Debug

# Remove build directory
clean:
	rm -rf build

# Build and run all tests using CMake/CTest
test: all
	cd build && ctest --output-on-failure

# Build and run the main application
run: all
	./build/thai_checkers_main
