#!/usr/bin/env bash
# Helper: run format hooks on staged files, restage modifications, then commit.
# Usage: scripts/commit_with_format.sh "commit message"
set -euo pipefail
msg=${1:-"chore: commit"}
FILES=$(git diff --name-only --cached --diff-filter=ACM | tr '\n' ' ')
if [ -z "$FILES" ]; then
  echo "No staged files to format. Running normal commit..."
  git commit -m "$msg"
  exit $?
fi
# Run pre-commit on the staged files
pre-commit run --files $FILES || true
# Restage any changes made by hooks
git add -u
# Commit if there are staged changes
if git diff --cached --quiet; then
  echo "No changes to commit after formatting."
else
  git commit -m "$msg"
fi
