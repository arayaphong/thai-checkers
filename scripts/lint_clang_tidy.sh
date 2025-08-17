#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# -----------------------------------------------------------------------------
# clang-tidy convenience wrapper
# Features:
#  * Auto-generate compile_commands.json (Debug) if missing
#  * Auto-detect clang-tidy binary (tries several versions)
#  * Parallel execution (-j / --jobs) using xargs (default: nproc)
#  * Restrict to changed (git staged / unstaged) files (--changed, --staged)
#  * Apply fixes (--fix) or export fixes (--export-fixes FILE)
#  * Additional custom clang-tidy args after '--'
#  * Optional environment variable: CLANG_TIDY_EXTRA_ARGS
# -----------------------------------------------------------------------------

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
cd "$REPO_ROOT"

usage() {
  cat <<EOF
Usage: $0 [options] [-- [extra clang-tidy args]]

Options:
  -j, --jobs N           Number of parallel jobs (default: CPU cores)
      --no-parallel      Disable parallel execution
      --changed          Only lint files changed vs HEAD (staged + unstaged)
      --staged           Only lint staged files
      --since REF        Only lint files changed since git ref (e.g. origin/main)
      --fix              Apply automatic fixes (-fix -fix-errors)
      --export-fixes F   Write YAML fixes to file F (implies no parallel)
      --list             Only list the files that would be linted, then exit
  --full             Disable concise defaults (show all diagnostics as before)
  --concise          Force concise mode (default unless --full is given)
  -v, --verbose          Verbose output
  -h, --help             Show this help

Environment:
  CLANG_TIDY            Override clang-tidy binary path
  CLANG_TIDY_EXTRA_ARGS Extra args appended (space separated)

Examples:
  $0                    # Lint all sources
  $0 --changed          # Only modified git files
  $0 -j 8 -- --checks='modernize-*'
  $0 --fix              # Apply fixes in-place
  $0 --export-fixes fixes.yaml
EOF
}

log() { echo "[lint] $*"; }
warn() { echo "[lint][warn] $*" >&2; }
die() { echo "[lint][error] $*" >&2; exit 1; }

JOBS=$(command -v nproc >/dev/null 2>&1 && nproc || echo 1)
PARALLEL=1
ONLY_CHANGED=0
ONLY_STAGED=0
SINCE_REF=""
APPLY_FIX=0
EXPORT_FIXES=""
LIST_ONLY=0
VERBOSE=0
FULL_MODE=0
FORCE_CONCISE=0

EXTRA_CLA_ARGS=()

PARSE_ARGS=1
while [[ $# -gt 0 ]]; do
  if [[ $PARSE_ARGS -eq 1 ]]; then
    case "$1" in
      -j|--jobs)
        shift || die "Missing value for --jobs"
        JOBS="$1";;
      --no-parallel)
        PARALLEL=0;;
      --changed)
        ONLY_CHANGED=1;;
      --staged)
        ONLY_STAGED=1;;
      --since)
        shift || die "Missing git ref after --since"
        SINCE_REF="$1";;
      --fix)
        APPLY_FIX=1; PARALLEL=0;;
      --export-fixes)
        shift || die "Missing filename after --export-fixes"
        EXPORT_FIXES="$1"; PARALLEL=0;;
      --list)
        LIST_ONLY=1;;
      --full)
        FULL_MODE=1;;
      --concise)
        FORCE_CONCISE=1;;
      -v|--verbose)
        VERBOSE=1;;
      -h|--help)
        usage; exit 0;;
      --)
        PARSE_ARGS=0; shift; continue;;
      *)
        if [[ $1 == -* ]]; then
          die "Unknown option: $1 (see --help)";
        else
          # treat as file? fallthrough; break loop
          break
        fi;;
    esac
    shift
  else
    EXTRA_CLA_ARGS+=("$1")
    shift
  fi
done

CLANG_TIDY_BIN=${CLANG_TIDY:-}
if [[ -z ${CLANG_TIDY_BIN} ]]; then
  for cand in clang-tidy clang-tidy-20 clang-tidy-19 clang-tidy-18 clang-tidy-17; do
    if command -v "$cand" >/dev/null 2>&1; then
      CLANG_TIDY_BIN="$cand"; break
    fi
  done
fi

if [[ -z "$CLANG_TIDY_BIN" ]]; then
  if command -v pre-commit >/dev/null 2>&1; then
    warn "clang-tidy not found, delegating to pre-commit hook..."
    exec pre-commit run clang-tidy --all-files
  fi
  die "clang-tidy not found in PATH (tried multiple versions)"
fi

# Ensure compile_commands.json exists
if [[ ! -f build/compile_commands.json ]]; then
  log "Generating compile_commands.json (Debug)"
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

if [[ ! -s build/compile_commands.json ]]; then
  die "build/compile_commands.json missing or empty"
fi

# Collect source files
mapfile -t ALL_SRCS < <(find src -type f \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' -o -name '*.c' \) | sort)

select_changed_files() {
  local base_ref="$1" mode="$2"; # mode: changed|staged
  local git_files
  case "$mode" in
    changed)
      if [[ -n $base_ref ]]; then
        git_files=$(git diff --name-only "$base_ref" --diff-filter=ACMR || true)
      else
        git_files=$(git diff --name-only HEAD --diff-filter=ACMR || true)
        git_files+=$'\n'"$(git diff --name-only --cached --diff-filter=ACMR || true)"
      fi;;
    staged)
      git_files=$(git diff --name-only --cached --diff-filter=ACMR || true);;
  esac
  awk 'NF' <<<"$git_files" | sort -u
}

FILES_TO_LINT=("${ALL_SRCS[@]}")
if [[ $ONLY_CHANGED -eq 1 || -n $SINCE_REF ]]; then
  mapfile -t changed < <(select_changed_files "${SINCE_REF}" changed)
  if [[ ${#changed[@]} -gt 0 ]]; then
    # filter intersection
    declare -A set
    for f in "${changed[@]}"; do set["$f"]=1; done
    FILES_TO_LINT=()
    for f in "${ALL_SRCS[@]}"; do [[ -n ${set[$f]:-} ]] && FILES_TO_LINT+=("$f"); done
  else
    FILES_TO_LINT=()
  fi
elif [[ $ONLY_STAGED -eq 1 ]]; then
  mapfile -t staged < <(select_changed_files "" staged)
  if [[ ${#staged[@]} -gt 0 ]]; then
    declare -A set
    for f in "${staged[@]}"; do set["$f"]=1; done
    FILES_TO_LINT=()
    for f in "${ALL_SRCS[@]}"; do [[ -n ${set[$f]:-} ]] && FILES_TO_LINT+=("$f"); done
  else
    FILES_TO_LINT=()
  fi
fi

if [[ ${#FILES_TO_LINT[@]} -eq 0 ]]; then
  log "No source files selected for linting."; exit 0
fi

if [[ $LIST_ONLY -eq 1 ]]; then
  printf '%s\n' "${FILES_TO_LINT[@]}"
  exit 0
fi

ARGS=("-p" "build")
[[ $APPLY_FIX -eq 1 ]] && ARGS+=("-fix" "-fix-errors")
[[ -n $EXPORT_FIXES ]] && ARGS+=("-export-fixes" "$EXPORT_FIXES")

if [[ -n ${CLANG_TIDY_EXTRA_ARGS:-} ]]; then
  # shellcheck disable=SC2206
  EXTRA_CLA_ARGS+=( ${CLANG_TIDY_EXTRA_ARGS} )
fi

# Provide concise defaults unless user opted into full mode or already specified checks/header-filter.
if [[ $FULL_MODE -eq 0 && -f .clang-tidy ]]; then
  # A project-wide .clang-tidy is present; do not inject overrides unless user forces custom args.
  :
elif [[ $FULL_MODE -eq 0 ]]; then
  has_checks=0; has_header_filter=0; has_warnings_as_errors=0
  for a in "${EXTRA_CLA_ARGS[@]}"; do
    [[ $a == -*checks* ]] && has_checks=1
    [[ $a == -header-filter=* ]] && has_header_filter=1
    [[ $a == -*warnings-as-errors* ]] && has_warnings_as_errors=1
  done
  if [[ $has_checks -eq 0 ]]; then
    # Focus on a curated subset of high-signal checks. Users can pass -- -checks='*' or --full to override.
    EXTRA_CLA_ARGS+=("-checks=readability-identifier-length,readability-magic-numbers,misc-no-recursion,misc-use-anonymous-namespace,modernize-use-nullptr,modernize-use-override")
  fi
  if [[ $has_header_filter -eq 0 ]]; then
    # Restrict diagnostics to project sources under src/ and include/.
    EXTRA_CLA_ARGS+=("-header-filter=^(src|include)/")
  fi
  if [[ $has_warnings_as_errors -eq 0 ]]; then
    # Treat selected warnings as errors to fail CI early; configurable.
    EXTRA_CLA_ARGS+=("-warnings-as-errors=readability-magic-numbers,readability-identifier-length")
  fi
  # Reduce noise from notes in third-party headers unless verbose.
  if [[ $VERBOSE -eq 0 ]]; then
    EXTRA_CLA_ARGS+=("-extra-arg=-fno-color-diagnostics")
  fi
  # Suppress file banners & notes for brevity.
  EXTRA_CLA_ARGS+=("-quiet")
fi

[[ $VERBOSE -eq 1 ]] && log "Using $CLANG_TIDY_BIN with args: ${ARGS[*]} ${EXTRA_CLA_ARGS[*]}"

run_one() {
  local file="$1"
  if [[ $VERBOSE -eq 1 ]]; then
    echo "==> $file"
  fi
  # The grep is to filter out warnings from clang-tidy about unsupported options
  # that might be present in a user's .ccls file, which some setups use.
  "$CLANG_TIDY_BIN" "${ARGS[@]}" "$file" "${EXTRA_CLA_ARGS[@]}" 2> >(grep -v "warning: ignoring unsupported" >&2)
}

STATUS=0
if [[ $PARALLEL -eq 1 ]]; then
  # To run in parallel, we need to make the `run_one` function and its required
  # variables (especially the arrays ARGS and EXTRA_CLA_ARGS) available to the
  # subshells that xargs will spawn.
  export -f run_one
  export CLANG_TIDY_BIN VERBOSE

  # The `declare -p` command prints the definition of a variable, which we can
  # then `eval` in the subshell to faithfully recreate it, including arrays.
  ARGS_DEF=$(declare -p ARGS)
  EXTRA_CLA_ARGS_DEF=$(declare -p EXTRA_CLA_ARGS)
  export ARGS_DEF EXTRA_CLA_ARGS_DEF

  printf '%s\n' "${FILES_TO_LINT[@]}" | xargs -P "$JOBS" -I{} bash -c '
    set -euo pipefail
    eval "$ARGS_DEF"
    eval "$EXTRA_CLA_ARGS_DEF"
    run_one "{}"
  ' || STATUS=$?
else
  for f in "${FILES_TO_LINT[@]}"; do
    run_one "$f" || STATUS=$?
  done
fi

exit $STATUS
