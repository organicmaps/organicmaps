#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

source "$REPO_ROOT/tools/hooks/format-config.bash"

if ! command -v ktlint >/dev/null 2>&1; then
  echo "Warning: ktlint not found, skipping Kotlin files."
  exit 0
fi

ktlint --version
echo "Running ktlint on Kotlin files..."

# ktlint runs on the JVM (~2s startup), so collect every matching file across
# all KTLINT_TARGETS first and pass them to a single ktlint invocation.
files=()
for entry in "${KTLINT_TARGETS[@]}"; do
  dir="${entry%%|*}"
  pattern="${entry##*|}"
  [ -d "$REPO_ROOT/$dir" ] || continue
  while IFS= read -r -d '' f; do
    files+=("$f")
  done < <(find "$REPO_ROOT/$dir" -type f -name "$pattern" -print0)
done

if [ ${#files[@]} -gt 0 ]; then
  ktlint --editorconfig="$REPO_ROOT/android/.editorconfig" --format "${files[@]}"
fi

git diff --exit-code
