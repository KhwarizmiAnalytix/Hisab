#!/bin/bash
# cmake-format script for Quarisma
# Formats all CMake files in the project according to .cmake-format.yaml
# Usage: bash Scripts/all-cmake-format.sh

set -e

# Get the repository root
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Find cmake-format executable
FMT=""
for fmt_cmd in cmake-format cmake-format-py; do
  if command -v "$fmt_cmd" >/dev/null 2>&1; then
    FMT="$fmt_cmd"
    break
  fi
done

if [ -z "$FMT" ]; then
  echo "Error: cmake-format not found in PATH" >&2
  echo "Install it with: pip install cmakelang" >&2
  exit 1
fi

echo "Using $FMT"

# After cmake-format, restore two-line section headers: the formatter reflows consecutive
# comments, merging a ruler line ("# =====...=====" or "# -----...-----") and "# Title" onto
# one line. Keep the ruler line bare (no title after the ruler) and put the title on the next
# comment line.
fix_hashruler_title_lines() {
  local cmake_file="$1"
  # 10+ '=' or '-' matches project-style dividers (equals: .cmake-format hashruler_min_length).
  # If cmake-format merged "# ruler" and "# title", splitting yields "# # title"; collapse to "# title".
  sed -i -E \
    -e 's/^# (={10,})[[:space:]]+(.+)$/# \1\n# \2/' \
    -e 's/^# (-{10,})[[:space:]]+(.+)$/# \1\n# \2/' \
    -e 's/^#(( )+#)+ /# /' \
    "$cmake_file"
}

# Find all CMake files (excluding ThirdParty and build directories).
# ThirdParty: explicit ! -path so vendored trees are never formatted (matches .lintrunner.toml).
echo "Scanning for CMake files..."
while IFS= read -r -d '' cmake_file; do
  "$FMT" -i --config-file=.cmake-format.yaml "$cmake_file"
  fix_hashruler_title_lines "$cmake_file"
done < <(
  find "$REPO_ROOT" \
    -type d \( -name .git -o -name .vscode -o -name .augment -o -name ThirdParty -o -name build -o -name 'build_*' -o -name dist \) -prune -false -o \
    -type f \( -name "CMakeLists.txt" -o -name "*.cmake" -o -name "*.cmake.in" \) \
    ! -path '*/ThirdParty/*' \
    -print0
)

echo "cmake-format complete."
