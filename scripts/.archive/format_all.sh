#!/usr/bin/env bash
# archived: replaced by pre-commit tasks
set -euo pipefail
if command -v pre-commit >/dev/null 2>&1; then
  pre-commit run clang-format --all-files
else
  echo "Use pre-commit to run formatting." >&2
fi
