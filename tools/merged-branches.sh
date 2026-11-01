#!/usr/bin/env bash
#
# Detect local branches merged into master, including squash-merged and rebased.
#
# Methods:
#   1. Ancestry — branch tip is ancestor of base (regular merge / fast-forward)
#   2. Cherry — uses git cherry for rebase detection (efficient per-branch)
#   3. Tree — merge simulation produces no diff (squash merge)
#
# Usage: merged-branches.sh [base-branch]   (default: master)
# Environment: PARALLEL=N controls parallelism (default: 8)

set -euo pipefail

BASE_BRANCH="${1:-master}"
PARALLEL="${PARALLEL:-8}"

if [[ ! "$PARALLEL" =~ ^[1-9][0-9]*$ ]]; then
  echo "Error: PARALLEL must be a positive integer." >&2
  exit 1
fi

if ! BASE_COMMIT=$(git rev-parse --verify --end-of-options "$BASE_BRANCH^{commit}" 2>/dev/null); then
  echo "Error: base branch '$BASE_BRANCH' not found." >&2
  exit 1
fi

BASE_REF=$(git rev-parse --symbolic-full-name --verify --end-of-options "$BASE_BRANCH")
BASE_TREE=$(git rev-parse "$BASE_COMMIT^{tree}")
ALL_BRANCHES=$(git for-each-ref --format='%(refname)' refs/heads/)

echo "Checking local branches against $BASE_BRANCH..." >&2

merged_file=$(mktemp)
phase1_remaining=$(mktemp)
unmerged_file=$(mktemp)
trap 'rm -f "$merged_file" "$phase1_remaining" "$unmerged_file"' EXIT

# ── Phase 1: ancestry check (fast, O(1) per branch) ──
phase1=0
remaining=0
while IFS= read -r ref; do
  [[ -z "$ref" || "$ref" == "$BASE_REF" ]] && continue
  branch="${ref#refs/heads/}"
  if git merge-base --is-ancestor "$ref" "$BASE_COMMIT"; then
    printf '%s  (merged)\n' "$branch" >> "$merged_file"
    phase1=$((phase1 + 1))
  else
    status=$?
    [[ "$status" -eq 1 ]] || exit "$status"
    printf '%s\0' "$branch" >> "$phase1_remaining"
    remaining=$((remaining + 1))
  fi
done <<< "$ALL_BRANCHES"
echo "  Phase 1 (ancestry): $phase1 merged, $remaining remaining" >&2

# ── Phase 2+3: combined cherry + merge-tree check (parallel) ──
# For each remaining branch:
#   - First try cheap cherry check (detects rebases)
#   - If that fails, try merge-tree (detects squash merges)
check_branch() {
  local branch="$1" base_commit="$2" base_tree="$3" merged="$4" unmerged="$5"
  local cherry merge_commits merge_tree status

  # Capture all output so a Git error or SIGPIPE cannot imply patch equivalence.
  cherry=$(git cherry "$base_commit" "refs/heads/$branch") || return
  if [[ "$cherry" != *"+"* ]]; then
    # Cherry omits merge commits, which may contain additional changes.
    merge_commits=$(git rev-list --merges --max-count=1 "$base_commit..refs/heads/$branch") || return
    if [[ -z "$merge_commits" ]]; then
      printf '%s  (rebased)\n' "$branch" >> "$merged"
      return
    fi
  fi

  # Tree check: simulate merge, see if it changes anything
  if merge_tree=$(git merge-tree --write-tree --no-messages "$base_commit" "refs/heads/$branch"); then
    if [[ "$merge_tree" == "$base_tree" ]]; then
      printf '%s  (squash-merged)\n' "$branch" >> "$merged"
      return
    fi
  else
    status=$?
    # A conflicted merge exits with 1; other errors must stop the scan.
    [[ "$status" -eq 1 ]] || return "$status"
  fi

  printf '%s\n' "$branch" >> "$unmerged"
}
export -f check_branch

echo "  Phase 2+3 (cherry + squash): checking $remaining branches with $PARALLEL workers..." >&2

if [[ "$remaining" -gt 0 ]]; then
  # Expand positional arguments in the worker shell.
  # shellcheck disable=SC2016
  xargs -0 -P "$PARALLEL" -I{} bash -c \
    'check_branch "$1" "$2" "$3" "$4" "$5"' _ {} "$BASE_COMMIT" "$BASE_TREE" "$merged_file" "$unmerged_file" \
    < "$phase1_remaining"
fi

total_merged=$(wc -l < "$merged_file" | tr -d ' ')
total_unmerged=0
[[ -s "$unmerged_file" ]] && total_unmerged=$(wc -l < "$unmerged_file" | tr -d ' ')
echo "  Done: $total_merged merged, $total_unmerged unmerged" >&2
echo "" >&2

# ── Output ──
if [[ "$total_merged" -gt 0 ]]; then
  echo "Branches merged into $BASE_BRANCH ($total_merged):"
  sort "$merged_file" | while IFS= read -r line; do
    echo "  $line"
  done
fi

if [[ "$total_unmerged" -gt 0 ]]; then
  echo ""
  echo "Unmerged branches ($total_unmerged):"
  sort "$unmerged_file" | while IFS= read -r line; do
    echo "  $line"
  done
fi
