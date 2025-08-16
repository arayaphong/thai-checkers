#!/bin/bash

# Thai Checkers 2 - Optimized Build & Run Script

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

APP_NAME="thai_checkers_main"
BUILD_DIR="build_optimized"
BIN_PATH="$BUILD_DIR/$APP_NAME"
ROOT_SYMLINK="thai_checkers_optimized"

# Build optimization options
OPT_ARCH="auto"       # auto|native|znver2|znver3|znver4
OPT_LTO="ON"          # ON|OFF
OPT_AGGRESSIVE=false  # add extra optimization flags

detect_cpu_arch() {
  # Auto-detect CPU architecture for optimal tuning
  local cpu_info
  if [[ -f /proc/cpuinfo ]]; then
    cpu_info=$(grep -m1 "model name" /proc/cpuinfo | cut -d: -f2 | xargs)

    # Check for AMD Zen architectures
    if echo "$cpu_info" | grep -qi "AMD"; then
      local cpu_family cpu_model
      cpu_family=$(grep -m1 "cpu family" /proc/cpuinfo | awk '{print $4}')
      cpu_model=$(grep -m1 "model" /proc/cpuinfo | awk '{print $3}')

      # AMD Zen architecture detection based on family/model
      if [[ "$cpu_family" == "25" ]]; then
        # Zen 3 (Ryzen 5000 series, EPYC 7003 series)
        echo "znver3"
      elif [[ "$cpu_family" == "23" ]]; then
        if [[ "$cpu_model" -ge 96 ]]; then
          # Zen 2 (Ryzen 3000/4000 series, EPYC 7002 series)
          echo "znver2"
        else
          # Zen/Zen+ (Ryzen 1000/2000 series, EPYC 7001 series)
          echo "znver1"
        fi
      elif [[ "$cpu_family" == "26" ]]; then
        # Zen 4 (Ryzen 7000 series, EPYC 9004 series)
        echo "znver4"
      else
        echo "native"
      fi
    else
      # For Intel or other CPUs, use native optimization
      echo "native"
    fi
  else
    # Fallback if /proc/cpuinfo is not available
    echo "native"
  fi
}

print_usage() {
  cat <<EOF
${BLUE}Thai Checkers 2 - Optimized Controller${NC}
Usage:
  $0 build [OPTIONS]        Build the optimized binary
  $0 run [ARGS...]          Run the optimized binary with optional ARGS

Build Options:
  --arch <auto|native|znver2|znver3|znver4>  CPU tuning target (default: auto)
  --lto <on|off>                             Enable Link Time Optimization (default: on)
  --aggressive                               Add extra optimization flags

Examples:
  $0 build                              # build with auto-detected CPU architecture
  $0 build --arch native --aggressive   # build with native arch and extra opts
  $0 run --timeout 30s                  # run with 30s timeout
  $0 run --help                         # show executable help

Notes:
  - Auto-detection works for AMD Zen architectures (znver1/2/3/4), defaults to 'native' for others
  - A symlink '${ROOT_SYMLINK}' is created for convenience
  - All arguments after 'run' are passed directly to the executable
EOF
}

ensure_symlink() {
  if [[ -f "$BIN_PATH" ]]; then
    ln -sf "$BIN_PATH" "$ROOT_SYMLINK"
  fi
}

build_optimized() {
  echo -e "${BLUE}Thai Checkers 2 - Optimized Build${NC}"
  echo "================================================"
  echo -e "${YELLOW}Cleaning previous optimized build...${NC}"
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"

  echo -e "${YELLOW}Configuring CMake with optimization...${NC}"

  # Map script options to CMake cache args
  local CMAKE_ARGS=(
    -S .
    -B "$BUILD_DIR"
    -DCMAKE_BUILD_TYPE=Release
  )

  # LTO toggle
  if [[ "${OPT_LTO^^}" == "ON" ]]; then
    CMAKE_ARGS+=( -DENABLE_LTO=ON )
  else
    CMAKE_ARGS+=( -DENABLE_LTO=OFF )
  fi

  # Architecture selection
  local actual_arch="$OPT_ARCH"
  if [[ "$OPT_ARCH" == "auto" ]]; then
    actual_arch=$(detect_cpu_arch)
    echo -e "${BLUE}Auto-detected CPU architecture: ${actual_arch}${NC}"
  fi

  if [[ "$actual_arch" == "native" ]]; then
    CMAKE_ARGS+=( -DENABLE_NATIVE_OPTIMIZATIONS=ON )
  else
    CMAKE_ARGS+=( -DAMD_ZEN_ARCH="$actual_arch" )
  fi

  # Compose release flags
  local BASE_RELEASE_FLAGS="-O3 -DNDEBUG -fomit-frame-pointer"
  local EXTRA_FLAGS=""
  if [[ "$OPT_AGGRESSIVE" == true ]]; then
    EXTRA_FLAGS+=" -ffast-math -funroll-loops -finline-functions"
  fi
  CMAKE_ARGS+=( -DCMAKE_CXX_FLAGS_RELEASE="$BASE_RELEASE_FLAGS $EXTRA_FLAGS" )
  CMAKE_ARGS+=( -DCMAKE_EXE_LINKER_FLAGS="-s" )

  cmake "${CMAKE_ARGS[@]}"

  echo -e "${YELLOW}Building optimized main application...${NC}"
  cmake --build "$BUILD_DIR" --config Release --target "$APP_NAME" -j"$(command -v nproc >/dev/null 2>&1 && nproc || echo 4)"

  if [[ -f "$BIN_PATH" ]]; then
    echo -e "${GREEN}✓ Optimized build completed successfully!${NC}"
    echo -e "${BLUE}Executable location: $(realpath "$BIN_PATH")${NC}"

    echo -e "\n${BLUE}Optimization Details:${NC}"
    if [[ "$OPT_ARCH" == "auto" ]]; then
      echo "- Arch: $actual_arch (auto-detected from: $OPT_ARCH)"
    else
      echo "- Arch: $actual_arch"
    fi
    echo "- LTO: ${OPT_LTO^^}"
    if [[ "$OPT_AGGRESSIVE" == true ]]; then
      echo "- Extras: -ffast-math -funroll-loops -finline-functions"
    else
      echo "- Extras: (none)"
    fi

    echo -e "\n${BLUE}Executable Info:${NC}"
    (cd "$BUILD_DIR" && ls -lh "$APP_NAME" && file "$APP_NAME")

    ensure_symlink
    echo -e "${GREEN}✓ Created/updated symlink: ${ROOT_SYMLINK}${NC}"
  else
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
  fi
}

run_optimized() {
  local -a ARGS=("$@")

  if [[ ! -f "$ROOT_SYMLINK" ]]; then
    if [[ -f "$BIN_PATH" ]]; then
      ensure_symlink
    else
      echo -e "${YELLOW}! Optimized executable not found. Building now...${NC}"
      build_optimized
    fi
  fi

  if [[ ! -f "$ROOT_SYMLINK" ]]; then
    echo -e "${RED}✗ Optimized executable not found!${NC}"
    echo -e "${BLUE}Run $0 build first to build the optimized version.${NC}"
    exit 1
  fi

  echo -e "${GREEN}Running Thai Checkers 2 (Optimized)...${NC}"
  echo "========================================"

  if command -v stdbuf >/dev/null 2>&1; then
    stdbuf -oL -eL "./$ROOT_SYMLINK" "${ARGS[@]}"
  else
    "./$ROOT_SYMLINK" "${ARGS[@]}"
  fi
}

# -------- Argument parsing --------
ACTION=""
APP_ARGS=()

if [[ $# -eq 0 ]]; then
  print_usage
  exit 0
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    build)
      ACTION="build"; shift ;;
    run)
      ACTION="run"; shift
      # All remaining arguments go to the app
      APP_ARGS+=("$@")
      break ;;
    --arch)
      OPT_ARCH="$2"; shift 2 ;;
    --arch=*)
      OPT_ARCH="${1#*=}"; shift ;;
    --lto)
      case "${2^^}" in
        ON|OFF) OPT_LTO="${2^^}";;
        on|off) OPT_LTO="${2^^}";;
        *) echo -e "${RED}Invalid --lto value: $2 (use on|off)${NC}"; exit 1;;
      esac
      shift 2 ;;
    --lto=*)
      case "${1#*=}" in
        on|ON) OPT_LTO=ON;;
        off|OFF) OPT_LTO=OFF;;
        *) echo -e "${RED}Invalid --lto value: ${1#*=} (use on|off)${NC}"; exit 1;;
      esac
      shift ;;
    --aggressive)
      OPT_AGGRESSIVE=true; shift ;;
    -h|--help)
      print_usage; exit 0 ;;
    *)
      echo -e "${RED}Unknown option: $1${NC}"
      print_usage
      exit 1 ;;
  esac
done

case "$ACTION" in
  build)
    build_optimized ;;
  run)
    run_optimized "${APP_ARGS[@]}" ;;
  *)
    echo -e "${RED}No action specified${NC}"
    print_usage
    exit 1 ;;
esac
