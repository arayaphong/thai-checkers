#!/usr/bin/env bash
set -euo pipefail

# Format only staged C/C++ files to keep diffs minimal
if ! command -v git >/dev/null 2>&1; then
  echo "git not found" >&2
  exit 127
fi
if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found" >&2
  exit 127
fi

# List staged files (Added, Copied, Modified, Renamed)
mapfile -t files < <(git diff --cached --name-only --diff-filter=ACMR | \
  grep -E '\\.(h|hpp|cpp)$' || true)

if [[ ${#files[@]} -eq 0 ]]; then
  exit 0
fi

# Format in place
clang-format -i "${files[@]}"

# Re-add potentially modified files to the index
git add "${files[@]}"

echo "clang-format applied to staged files:" >&2
printf ' - %s\n' "${files[@]}" >&2
