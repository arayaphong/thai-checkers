#!/usr/bin/env bash
set -euo pipefail

declare -a roots=("include" "src")

# Ensure clang-format is available
if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found in PATH" >&2
  exit 127
fi

# Ensure .clang-format exists at the repo root
if [[ ! -f .clang-format ]]; then
  echo ".clang-format not found at repo root" >&2
  exit 2
fi

# Format all C/C++ files under include/ and src/
find "${roots[@]}" -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) -print0 \
  | xargs -0 -r -n 50 clang-format -i

echo "Formatted all C/C++ files under: ${roots[*]}"
