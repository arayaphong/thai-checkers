#!/bin/bash

# Thai Checkers 2 - Quick Run Script for Optimized Build
# This script runs the optimized version of the application

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if optimized executable exists
if [ ! -f "thai_checkers_optimized" ]; then
    echo -e "${RED}✗ Optimized executable not found!${NC}"
    echo -e "${BLUE}Run ./build_optimized.sh first to build the optimized version.${NC}"
    exit 1
fi

echo -e "${GREEN}Running Thai Checkers 2 (Optimized)...${NC}"
echo "========================================"

# Run the optimized application with any passed arguments
./thai_checkers_optimized "$@"
