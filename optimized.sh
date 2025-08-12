#!/bin/bash

# Thai Checkers 2 - Unified Optimized Script
# Combines build + run + profiling into a single entrypoint

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
SCRIPT_NO_QUIET=false
SCRIPT_PROFILER="auto" # auto|perf|callgrind|gprof
# Optimization toggles (mapped to CMake)
SCRIPT_ARCH="native"   # native|znver2|znver3|znver4
SCRIPT_LTO="ON"        # ON|OFF
SCRIPT_AGGRESSIVE=false # add -ffast-math/-funroll-loops/-finline-functions
SCRIPT_THREADS=""      # OMP_NUM_THREADS value when running
SCRIPT_TIMEOUT=""      # timeout in seconds, empty means infinity

print_usage() {
  cat <<EOF
${BLUE}Thai Checkers 2 - Optimized Controller${NC}
Usage:
  $0 --build [-- ARGS...]       Build the optimized binary (ignores ARGS)
  $0 --run   [-- ARGS...]       Run the optimized binary with optional ARGS
  $0 --profiling [-- ARGS...]   Run with CPU profiling (perf/valgrind/gprof)

Options:
  --profiler <perf|callgrind|gprof>   Force a specific profiler (default: auto)
  --no-quiet                          Do not inject --quiet into app args during profiling
  --arch <native|znver2|znver3|znver4>  CPU tuning target (default: native)
  --lto <on|off>                       Enable Link Time Optimization (default: on)
  --aggressive                         Add extra opts: -ffast-math -funroll-loops -finline-functions
  --threads <N>                        Set OMP_NUM_THREADS when running
  --timeout <N>                        Set execution timeout in seconds (default: infinity)

Examples:
  $0 --run                                   # run with default settings
  $0 --run --timeout 30                     # run with 30s timeout
  $0 --profiling                             # profile quietly by default
  $0 --profiling --profiler callgrind        # force Valgrind/Callgrind
  $0 --profiling --no-quiet                 # profile verbosely
  $0 --profiling --timeout 60               # profile with 60s timeout

Notes:
  - A symlink '${ROOT_SYMLINK}' is created to point at ${BIN_PATH} for convenience.
  - Use "--" to separate script options from app arguments.
  - Profiling defaults to quiet mode to reduce I/O overhead; pass --no-quiet to this script to disable.
EOF
}

ensure_symlink() {
  # Create or refresh root-level convenience symlink
  if [[ -f "$BIN_PATH" ]]; then
    ln -sf "$BIN_PATH" "$ROOT_SYMLINK"
  fi
}

run_with_timeout() {
  # Helper function to run commands with optional timeout
  # Usage: run_with_timeout <timeout_seconds> <command> [args...]
  # If timeout_seconds is empty or "infinity", runs without timeout
  local timeout_val="$1"
  shift

  if [[ -z "$timeout_val" || "$timeout_val" == "infinity" ]]; then
    # No timeout, run directly
    "$@"
  else
    # Use timeout command if available
    if command -v timeout >/dev/null 2>&1; then
      echo -e "${YELLOW}Running with ${timeout_val}s timeout...${NC}"
      timeout "$timeout_val" "$@"
    else
      echo -e "${YELLOW}Warning: 'timeout' command not available, running without timeout...${NC}"
      "$@"
    fi
  fi
}

build_optimized() {
  local for_profile=${1:-false}

  echo -e "${BLUE}Thai Checkers 2 - Optimized Build${NC}"
  echo "================================================"
  echo -e "${YELLOW}Cleaning previous optimized build...${NC}"
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"

  echo -e "${YELLOW}Configuring CMake with ${for_profile:+profiling-friendly }optimization...${NC}"

  # Map script options to CMake cache args
  local CMAKE_ARGS=(
    -S .
    -B "$BUILD_DIR"
    -DCMAKE_BUILD_TYPE=Release
  )

  # LTO toggle
  if [[ "${SCRIPT_LTO^^}" == "ON" ]]; then
    CMAKE_ARGS+=( -DENABLE_LTO=ON )
  else
    CMAKE_ARGS+=( -DENABLE_LTO=OFF )
  fi

  # Architecture selection: prefer explicit znverX over native
  if [[ "$SCRIPT_ARCH" == "native" ]]; then
    CMAKE_ARGS+=( -DENABLE_NATIVE_OPTIMIZATIONS=ON )
  else
    CMAKE_ARGS+=( -DAMD_ZEN_ARCH="$SCRIPT_ARCH" )
  fi

  # Compose additional release flags
  local BASE_RELEASE_FLAGS="-O3 -DNDEBUG"
  local EXTRA_FLAGS=""
  if [[ "$SCRIPT_AGGRESSIVE" == true ]]; then
    EXTRA_FLAGS+=" -ffast-math -funroll-loops -finline-functions"
  fi
  if [[ "$for_profile" == true ]]; then
    EXTRA_FLAGS+=" -g -fno-omit-frame-pointer"
  else
    EXTRA_FLAGS+=" -fomit-frame-pointer"
  fi
  CMAKE_ARGS+=( -DCMAKE_CXX_FLAGS_RELEASE="$BASE_RELEASE_FLAGS $EXTRA_FLAGS" )
  # Strip binary in non-profile builds
  if [[ "$for_profile" == true ]]; then
    CMAKE_ARGS+=( -DCMAKE_EXE_LINKER_FLAGS="" )
  else
    CMAKE_ARGS+=( -DCMAKE_EXE_LINKER_FLAGS="-s" )
  fi

  cmake "${CMAKE_ARGS[@]}"

  echo -e "${YELLOW}Building optimized main application...${NC}"
  cmake --build "$BUILD_DIR" --config Release --target "$APP_NAME" -j"$(command -v nproc >/dev/null 2>&1 && nproc || echo 4)"

  if [[ -f "$BIN_PATH" ]]; then
    echo -e "${GREEN}✓ Optimized build completed successfully!${NC}"
    echo -e "${BLUE}Executable location: $(realpath "$BIN_PATH")${NC}"

    echo -e "\n${BLUE}Optimization Details:${NC}"
    if [[ "$SCRIPT_ARCH" == "native" ]]; then
      echo "- Arch: native"
    else
      echo "- Arch: $SCRIPT_ARCH"
    fi
    echo "- LTO: ${SCRIPT_LTO^^}"
    if [[ "$SCRIPT_AGGRESSIVE" == true ]]; then
      echo "- Extras: -ffast-math -funroll-loops -finline-functions"
    else
      echo "- Extras: (none)"
    fi
    if [[ "$for_profile" == true ]]; then
      echo "- Frame pointer: kept (profiling-friendly)"
      echo "- Debug symbols: enabled"
      echo "- Binary stripped: no"
    else
      echo "- Frame pointer: omitted"
      echo "- Binary stripped: yes"
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
      build_optimized false
    fi
  fi

  if [[ ! -f "$ROOT_SYMLINK" ]]; then
    echo -e "${RED}✗ Optimized executable not found!${NC}"
    echo -e "${BLUE}Run $0 --build first to build the optimized version.${NC}"
    exit 1
  fi

  echo -e "${GREEN}Running Thai Checkers 2 (Optimized)...${NC}"
  echo "========================================"
  local RUN_ENV=()
  if [[ -n "$SCRIPT_THREADS" ]]; then
    RUN_ENV+=( OMP_NUM_THREADS="$SCRIPT_THREADS" )
  fi
  if command -v stdbuf >/dev/null 2>&1; then
    run_with_timeout "$SCRIPT_TIMEOUT" "${RUN_ENV[@]}" stdbuf -oL -eL "./$ROOT_SYMLINK" "${ARGS[@]}"
  else
    run_with_timeout "$SCRIPT_TIMEOUT" "${RUN_ENV[@]}" "./$ROOT_SYMLINK" "${ARGS[@]}"
  fi
}

profile_optimized() {
  local -a ARGS=("$@")
  # Default to quiet unless explicitly disabled at script level or already present in app args
  local -a RUN_ARGS=()
  if [[ "$SCRIPT_NO_QUIET" == true ]]; then
    RUN_ARGS=("${ARGS[@]}")
  else
    local have_quiet=false
    for a in "${ARGS[@]}"; do
      if [[ "$a" == "--quiet" || "$a" == "-q" ]]; then have_quiet=true; break; fi
    done
    if [[ "$have_quiet" == true ]]; then
      RUN_ARGS=("${ARGS[@]}")
    else
      RUN_ARGS=("${ARGS[@]}" "--quiet")
    fi
  fi
  # Ensure a profiling-friendly build is present
  echo -e "${YELLOW}Preparing profiling-friendly build...${NC}"
  build_optimized true

  echo -e "${BLUE}Profiling Thai Checkers 2...${NC}"

  # If user forces Callgrind, honor it immediately.
  if [[ "$SCRIPT_PROFILER" == "callgrind" ]]; then
    if command -v valgrind >/dev/null 2>&1 && command -v callgrind_annotate >/dev/null 2>&1; then
      echo -e "${YELLOW}Using Valgrind Callgrind (forced).${NC}"
      local CG_OUT="$BUILD_DIR/callgrind.out"
      run_with_timeout "$SCRIPT_TIMEOUT" valgrind --tool=callgrind --callgrind-out-file="$CG_OUT" "./$ROOT_SYMLINK" "${RUN_ARGS[@]}"
      echo -e "${GREEN}✓ Callgrind run complete. Annotated summary (top ~60 lines)...${NC}"
      callgrind_annotate "$CG_OUT" | head -n 60 || true
      echo -e "${BLUE}Callgrind output at: $CG_OUT${NC}"
      return
    else
      echo -e "${RED}Valgrind/Callgrind not available.${NC}"
      echo -e "${BLUE}Install with: sudo apt install valgrind kcachegrind${NC}"
      exit 2
    fi
  fi

  # Prefer kernel-matched perf if available; else PATH perf. Fall back to Valgrind if unavailable.
  local PERF_BIN=""
  local PERF_KDIR_GENERIC="/usr/lib/linux-tools-$(uname -r)-generic"
  local PERF_KDIR="/usr/lib/linux-tools-$(uname -r)"
  local PERF_SCAN_DIR="/usr/lib/linux-tools"
  if [[ -x "$PERF_KDIR_GENERIC/perf" ]]; then
    PERF_BIN="$PERF_KDIR_GENERIC/perf"
  elif [[ -x "$PERF_KDIR/perf" ]]; then
    PERF_BIN="$PERF_KDIR/perf"
  elif command -v perf >/dev/null 2>&1; then
    PERF_BIN="$(command -v perf)"
  fi

  # If still not set or wrapper points to missing version, scan all linux-tools for any perf as a last resort
  if [[ -z "$PERF_BIN" || "$PERF_BIN" == "/usr/bin/perf" ]]; then
    if [[ -d "$PERF_SCAN_DIR" ]]; then
      # Pick the newest available perf by versioned directory name
      local ANY_PERF
      ANY_PERF=$(find "$PERF_SCAN_DIR" -maxdepth 2 -type f -name perf 2>/dev/null | sort -V | tail -n 1 || true)
      if [[ -n "$ANY_PERF" ]]; then
        PERF_BIN="$ANY_PERF"
      fi
    fi
  fi

  if [[ -n "$PERF_BIN" && ( "$SCRIPT_PROFILER" == "auto" || "$SCRIPT_PROFILER" == "perf" ) ]]; then
    echo -e "${YELLOW}Using 'perf' for profiling (record + report).${NC}"
    echo -e "Resolved perf binary: ${BLUE}$PERF_BIN${NC}"
    # Quick advisory if kernel settings block perf
    local PERF_PARANOID
    PERF_PARANOID=$(sysctl -n kernel.perf_event_paranoid 2>/dev/null || echo "")
    if [[ -n "$PERF_PARANOID" && "$PERF_PARANOID" -gt 2 ]]; then
      echo -e "${YELLOW}Note: kernel.perf_event_paranoid=$PERF_PARANOID may block perf for non-root.${NC}"
      echo -e "Hint: try 'sudo sysctl kernel.perf_event_paranoid=1' and 'sudo sysctl kernel.kptr_restrict=0' if recording fails."
    fi
    local PERF_OUT="$BUILD_DIR/perf.data"
  if run_with_timeout "$SCRIPT_TIMEOUT" env OMP_NUM_THREADS="${SCRIPT_THREADS:-}" "$PERF_BIN" record -g --call-graph=dwarf -F 99 -o "$PERF_OUT" -- "./$ROOT_SYMLINK" "${RUN_ARGS[@]}"; then
      echo -e "${GREEN}✓ perf record complete. Generating report (top ~60 lines)...${NC}"
      "$PERF_BIN" report --stdio -i "$PERF_OUT" | head -n 60 || true
      echo -e "${BLUE}Full perf data at: $PERF_OUT${NC}"
      return
    else
      echo -e "${RED}perf failed or is not compatible with the current kernel/tools.${NC}"
      # If we used the wrapper, try a discovered versioned perf once as a second chance
  if [[ "$PERF_BIN" == "/usr/bin/perf" && -d "$PERF_SCAN_DIR" ]]; then
        local ALT_PERF
        ALT_PERF=$(find "$PERF_SCAN_DIR" -maxdepth 2 -type f -name perf 2>/dev/null | sort -V | tail -n 1 || true)
        if [[ -n "$ALT_PERF" && "$ALT_PERF" != "$PERF_BIN" ]]; then
          echo -e "${YELLOW}Retrying with discovered perf binary: ${BLUE}$ALT_PERF${NC}"
          if run_with_timeout "$SCRIPT_TIMEOUT" env OMP_NUM_THREADS="${SCRIPT_THREADS:-}" "$ALT_PERF" record -g --call-graph=dwarf -F 99 -o "$PERF_OUT" -- "./$ROOT_SYMLINK" "${RUN_ARGS[@]}"; then
            echo -e "${GREEN}✓ perf record complete (alt). Generating report (top ~60 lines)...${NC}"
            "$ALT_PERF" report --stdio -i "$PERF_OUT" | head -n 60 || true
            echo -e "${BLUE}Full perf data at: $PERF_OUT${NC}"
            return
          fi
        fi
      fi
      echo -e "${YELLOW}Attempting fallback to Valgrind Callgrind...${NC}"
    fi
  fi

  if [[ "$SCRIPT_PROFILER" != "gprof" ]] && command -v valgrind >/dev/null 2>&1 && command -v callgrind_annotate >/dev/null 2>&1; then
    echo -e "${YELLOW}Using Valgrind Callgrind for profiling.${NC}"
    local CG_OUT="$BUILD_DIR/callgrind.out"
  run_with_timeout "$SCRIPT_TIMEOUT" env OMP_NUM_THREADS="${SCRIPT_THREADS:-}" valgrind --tool=callgrind --callgrind-out-file="$CG_OUT" "./$ROOT_SYMLINK" "${RUN_ARGS[@]}"
    echo -e "${GREEN}✓ Callgrind run complete. Annotated summary (top ~60 lines)...${NC}"
    callgrind_annotate "$CG_OUT" | head -n 60 || true
    echo -e "${BLUE}Callgrind output at: $CG_OUT${NC}"
  else
    # Final fallback: gprof (-pg) if available
    if command -v gprof >/dev/null 2>&1; then
      echo -e "${YELLOW}Using gprof fallback (instrumented -pg build).${NC}"
      local GPROF_BUILD_DIR="build_gprof"
      local GPROF_BIN_PATH="$GPROF_BUILD_DIR/$APP_NAME"

      echo -e "${YELLOW}Configuring CMake for gprof (-pg) in $GPROF_BUILD_DIR...${NC}"
      rm -rf "$GPROF_BUILD_DIR"
      mkdir -p "$GPROF_BUILD_DIR"
      # Use moderate opts; disable IPO/LTO which can confuse gprof, keep symbols and frame pointers
      cmake -S . \
        -B "$GPROF_BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS_RELEASE="-O2 -g -fno-omit-frame-pointer -pg" \
        -DCMAKE_EXE_LINKER_FLAGS="-pg" \
        -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF

      echo -e "${YELLOW}Building gprof-instrumented binary...${NC}"
      cmake --build "$GPROF_BUILD_DIR" --config Release --target "$APP_NAME" -j"$(command -v nproc >/dev/null 2>&1 && nproc || echo 4)"

      if [[ -f "$GPROF_BIN_PATH" ]]; then
        echo -e "${GREEN}✓ Build complete. Running to generate gmon.out...${NC}"
        (
          cd "$GPROF_BUILD_DIR" && \
          run_with_timeout "$SCRIPT_TIMEOUT" env OMP_NUM_THREADS="${SCRIPT_THREADS:-}" stdbuf -oL -eL "./$APP_NAME" "${RUN_ARGS[@]}" || true
        )
        if [[ -f "$GPROF_BUILD_DIR/gmon.out" ]]; then
          echo -e "${GREEN}✓ Profiling data generated. Printing gprof summary (top ~100 lines)...${NC}"
          gprof "$GPROF_BIN_PATH" "$GPROF_BUILD_DIR/gmon.out" | head -n 100 || true
          echo -e "${BLUE}Full gprof output available via:${NC}"
          echo "  gprof $GPROF_BIN_PATH $GPROF_BUILD_DIR/gmon.out | less"
        else
          echo -e "${RED}✗ gmon.out not found after run. The program may require input or did not exit cleanly.${NC}"
          echo -e "${BLUE}Try running manually, then run gprof:${NC}"
          echo "  (cd $GPROF_BUILD_DIR && ./$APP_NAME [args])"
          echo "  gprof $GPROF_BIN_PATH $GPROF_BUILD_DIR/gmon.out | less"
          exit 2
        fi
      else
        echo -e "${RED}✗ Failed to build gprof-instrumented binary.${NC}"
        exit 2
      fi
    else
      echo -e "${RED}✗ No supported profiling tools found or usable.${NC}"
      echo -e "${BLUE}To enable profiling, install one of:${NC}"
      echo "  - perf:     sudo apt install linux-tools-\$(uname -r) linux-cloud-tools-\$(uname -r)"
      echo "  - valgrind: sudo apt install valgrind"
      echo "  - gprof:    sudo apt install binutils"
      echo -e "${BLUE}You can still run the optimized build with: $0 --run${NC}"
      exit 2
    fi
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
    --build)
      ACTION="build"; shift ;;
    --run)
      ACTION="run"; shift ;;
    --profiling|--profile)
      ACTION="profiling"; shift ;;
    --profiler)
      SCRIPT_PROFILER="$2"; shift 2 ;;
    --profiler=*)
      SCRIPT_PROFILER="${1#*=}"; shift ;;
    --callgrind)
      SCRIPT_PROFILER="callgrind"; shift ;;
    --perf)
      SCRIPT_PROFILER="perf"; shift ;;
    --gprof)
      SCRIPT_PROFILER="gprof"; shift ;;
    --no-quiet)
      SCRIPT_NO_QUIET=true; shift ;;
    --arch)
      SCRIPT_ARCH="$2"; shift 2 ;;
    --arch=*)
      SCRIPT_ARCH="${1#*=}"; shift ;;
    --lto)
      case "${2^^}" in
        ON|OFF) SCRIPT_LTO="${2^^}";;
        on|off) SCRIPT_LTO="${2^^}";;
        *) echo -e "${RED}Invalid --lto value: $2 (use on|off)${NC}"; exit 1;;
      esac
      shift 2 ;;
    --lto=*)
      case "${1#*=}" in
        on|ON) SCRIPT_LTO=ON;;
        off|OFF) SCRIPT_LTO=OFF;;
        *) echo -e "${RED}Invalid --lto value: ${1#*=} (use on|off)${NC}"; exit 1;;
      esac
      shift ;;
    --aggressive)
      SCRIPT_AGGRESSIVE=true; shift ;;
    --threads)
      SCRIPT_THREADS="$2"; shift 2 ;;
    --threads=*)
      SCRIPT_THREADS="${1#*=}"; shift ;;
    --timeout)
      SCRIPT_TIMEOUT="$2"; shift 2 ;;
    --timeout=*)
      SCRIPT_TIMEOUT="${1#*=}"; shift ;;
    -h|--help)
      print_usage; exit 0 ;;
    --)
      shift
      APP_ARGS+=("$@")
      break ;;
    *)
      # Treat trailing tokens as app args once an action is chosen
      if [[ -n "$ACTION" ]]; then
        APP_ARGS+=("$1")
        shift
      else
        echo -e "${RED}Unknown option: $1${NC}"
        print_usage
        exit 1
      fi
      ;;
  esac

done

case "$ACTION" in
  build)
    build_optimized false ;;
  run)
    run_optimized "${APP_ARGS[@]}" ;;
  profiling)
    profile_optimized "${APP_ARGS[@]}" ;;
  *)
    print_usage
    exit 1 ;;

esac
