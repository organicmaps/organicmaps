#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

# Shared list of files and extensions to format, defined in a separate config file for reuse across hooks and tools
source "$REPO_ROOT/tools/hooks/format-config.bash"

XARGS_OPTS="-n1 -0 -P0"
if xargs -r </dev/null 2>/dev/null; then
  XARGS_OPTS="-r $XARGS_OPTS"
fi

CLANG_FORMAT=$(resolve_clang_format)
"$CLANG_FORMAT" --version
echo "Running clang-format on all repository files..."

# Only tracked files: build output and untracked work in progress are skipped.
# Files that must not be formatted are in .clang-format-ignore.
git ls-files -z -- "${CLANG_FORMAT_TARGETS[@]}" | xargs $XARGS_OPTS "$CLANG_FORMAT" -i

# Swift files (if swiftformat is available)
if command -v swiftformat >/dev/null 2>&1; then
  echo "Running swiftformat on Swift files..."
  for dir in "${SWIFTFORMAT_TARGETS[@]}"; do
    [ -d "$dir" ] || continue
    swiftformat "$dir"
  done
else
  echo "Warning: swiftformat not found, skipping Swift files."
fi

git diff --exit-code
