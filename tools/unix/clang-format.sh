#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Shared list of files and extensions to format, defined in a separate config file for reuse across hooks and tools
source "$REPO_ROOT/tools/hooks/format-config.bash"

XARGS_OPTS="-n1 -0 -P0"
if xargs -r </dev/null 2>/dev/null; then
  XARGS_OPTS="-r $XARGS_OPTS"
fi

CLANG_FORMAT=$(resolve_clang_format)
"$CLANG_FORMAT" --version
echo "Running clang-format on all repository files..."

for entry in "${CLANG_FORMAT_TARGETS[@]}"; do
  dir="${entry%%|*}"
  pattern="${entry##*|}"
  [ -d "$REPO_ROOT/$dir" ] || continue
  find "$REPO_ROOT/$dir" -type f -name "$pattern" -print0 \
    | xargs $XARGS_OPTS "$CLANG_FORMAT" -i
done

# Swift files (if swiftformat is available)
if command -v swiftformat >/dev/null 2>&1; then
  echo "Running swiftformat on Swift files..."
  for entry in "${SWIFTFORMAT_TARGETS[@]}"; do
    dir="${entry%%|*}"
    pattern="${entry##*|}"
    [ -d "$REPO_ROOT/$dir" ] || continue
    find "$REPO_ROOT/$dir" -type f -name "$pattern" -print0 \
      | xargs $XARGS_OPTS swiftformat
  done
else
  echo "Warning: swiftformat not found, skipping Swift files."
fi

git diff --exit-code
