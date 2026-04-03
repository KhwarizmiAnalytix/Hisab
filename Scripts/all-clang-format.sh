#!/bin/bash

set -euo pipefail

# Determine repository root (fallback to script dir/.. if git not available)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if command -v git >/dev/null 2>&1 && git -C "$SCRIPT_DIR" rev-parse --show-toplevel >/dev/null 2>&1; then
  REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"
else
  REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
fi

# Variable that will hold the name of the clang-format command
FMT=""

# Prefer unversioned clang-format; fall back to the highest versioned one found in PATH
if command -v clang-format >/dev/null 2>&1; then
  FMT="clang-format"
else
  # Find all clang-format-N / clang-format-N.M binaries in PATH, pick the highest version
  FMT="$(compgen -c clang-format- 2>/dev/null \
    | sort -t- -k3 -V -r \
    | while read -r candidate; do
        command -v "$candidate" >/dev/null 2>&1 && echo "$candidate" && break
      done)"
fi

if [ -z "$FMT" ]; then
  echo "Error: failed to find clang-format in PATH" >&2
  exit 1
fi

echo "Using $FMT"

# Collect files to format
if [[ "${1:-}" == "--all" ]]; then
  # Find all C++ headers/sources: .cpp, .hxx, .h (exclude vendor/build and VCS dirs)
  echo "Scanning for .cpp, .hxx, .h files under $REPO_ROOT ..."
  mapfile -d '' FILES < <(find "$REPO_ROOT" \
    -type d \( -name .git -o -name .vscode -o -name .augment -o -name ThirdParty -o -name venv -o -name build -o -name 'build_*' -o -name dist \) -prune -false -o \
    -type f \( -name "*.cpp" -o -name "*.hxx" -o -name "*.h" \) -print0)
else
  # Only format files changed relative to HEAD (staged + unstaged)
  echo "Formatting only changed files (use --all to format everything) ..."
  mapfile -t CHANGED < <(git -C "$REPO_ROOT" diff --name-only HEAD 2>/dev/null; git -C "$REPO_ROOT" diff --name-only 2>/dev/null; git -C "$REPO_ROOT" ls-files --others --exclude-standard 2>/dev/null)
  # Deduplicate, filter extensions, make absolute paths, check existence
  declare -A SEEN
  FILES=()
  for f in "${CHANGED[@]}"; do
    [[ "${SEEN[$f]+_}" ]] && continue
    SEEN[$f]=1
    case "$f" in *.cpp|*.hxx|*.h) ;; *) continue ;; esac
    abs="$REPO_ROOT/$f"
    [[ -f "$abs" ]] && FILES+=("$abs")
  done
fi

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No files to format."
  exit 0
fi

echo "Formatting ${#FILES[@]} file(s) ..."
printf '%s\n' "${FILES[@]}"
printf '%s\0' "${FILES[@]}" | xargs -0 -I{} "$FMT" -i {}

echo "clang-format complete."
