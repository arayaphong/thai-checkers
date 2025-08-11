#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found in PATH" >&2
  exit 127
fi

# Check mode: exit non-zero if any file would be reformatted
find include src -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) -print0 \
  | xargs -0 clang-format -n --Werror
