#!/bin/bash

# Thai Checkers 2 - Optimized Build Script
# This script builds the main application with extreme optimization

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Thai Checkers 2 - Optimized Build Script${NC}"
echo "================================================"

# Clean previous optimized build
echo -e "${YELLOW}Cleaning previous optimized build...${NC}"
rm -rf build_optimized

# Create optimized build directory
mkdir -p build_optimized
cd build_optimized

# Configure with extreme optimization
echo -e "${YELLOW}Configuring CMake with extreme optimization...${NC}"
cmake -S .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -march=native -mtune=native -flto -ffast-math -funroll-loops -finline-functions -fomit-frame-pointer -DNDEBUG" \
    -DCMAKE_EXE_LINKER_FLAGS="-flto -s" \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# Build the main application
echo -e "${YELLOW}Building optimized main application...${NC}"
cmake --build . --config Release --target thai_checkers_main -j$(nproc)

# Check if build was successful
if [ -f "thai_checkers_main" ]; then
    echo -e "${GREEN}✓ Optimized build completed successfully!${NC}"
    echo -e "${BLUE}Executable location: $(pwd)/thai_checkers_main${NC}"
    
    # Show file size and optimization info
    echo -e "\n${BLUE}Optimization Details:${NC}"
    echo "- March: native (optimized for current CPU)"
    echo "- Mtune: native (tuned for current CPU)"
    echo "- LTO: enabled (Link Time Optimization)"
    echo "- Fast math: enabled"
    echo "- Loop unrolling: enabled"
    echo "- Inlining: aggressive"
    echo "- Frame pointer: omitted"
    echo "- Binary stripped: yes"
    
    # Show executable info
    echo -e "\n${BLUE}Executable Info:${NC}"
    ls -lh thai_checkers_main
    file thai_checkers_main
    
    # Create a symlink in the project root for easy access
    cd ..
    ln -sf build_optimized/thai_checkers_main thai_checkers_optimized
    echo -e "${GREEN}✓ Created symlink: thai_checkers_optimized${NC}"
    
else
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi

echo -e "\n${GREEN}Ready to run with: ./thai_checkers_optimized${NC}"
