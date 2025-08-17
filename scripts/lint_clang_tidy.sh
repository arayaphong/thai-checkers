#!/usr/bin/env bash
set -euo pipefail

# Ensure compile_commands.json exists
if [[ ! -f build/compile_commands.json ]]; then
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

# Pick an available clang-tidy binary
CLANG_TIDY_BIN=""
for cand in clang-tidy clang-tidy-20 clang-tidy-19 clang-tidy-18 clang-tidy-17; do
  if command -v "$cand" >/dev/null 2>&1; then
    CLANG_TIDY_BIN="$cand"
    break
  fi
done

if [[ -z "$CLANG_TIDY_BIN" ]]; then
  if command -v pre-commit >/dev/null 2>&1; then
    echo "clang-tidy not found, using pre-commit hook instead..." >&2
    exec pre-commit run clang-tidy --all-files
  fi
  echo "clang-tidy not found in PATH (tried: clang-tidy, clang-tidy-20, -19, -18, -17)" >&2
  exit 127
fi

# Use compile database and analyze only translation units; headers are covered via includes
ARGS=("-p" "build")

find src -type f \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' -o -name '*.c' \) -print0 \
  | xargs -0 -n 1 "$CLANG_TIDY_BIN" "${ARGS[@]}"
