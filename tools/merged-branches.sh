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

if ! git rev-parse --verify "$BASE_BRANCH" &>/dev/null; then
  echo "Error: base branch '$BASE_BRANCH' not found." >&2
  exit 1
fi

BASE_TREE=$(git rev-parse "$BASE_BRANCH^{tree}")

mapfile -t ALL_BRANCHES < <(git for-each-ref --format='%(refname:short)' refs/heads/ | grep -vxF "$BASE_BRANCH")
TOTAL=${#ALL_BRANCHES[@]}
echo "Checking $TOTAL branches against $BASE_BRANCH..." >&2

merged_file=$(mktemp)
phase1_remaining=$(mktemp)
phase2_remaining=$(mktemp)
unmerged_file=$(mktemp)
trap 'rm -f "$merged_file" "$phase1_remaining" "$phase2_remaining" "$unmerged_file"' EXIT

# ── Phase 1: ancestry check (fast, O(1) per branch) ──
for branch in "${ALL_BRANCHES[@]}"; do
  if git merge-base --is-ancestor "$branch" "$BASE_BRANCH" 2>/dev/null; then
    echo "$branch  (merged)" >> "$merged_file"
  else
    echo "$branch" >> "$phase1_remaining"
  fi
done
phase1=$(wc -l < "$merged_file" | tr -d ' ')
remaining=$(wc -l < "$phase1_remaining" | tr -d ' ')
echo "  Phase 1 (ancestry): $phase1 merged, $remaining remaining" >&2

# ── Phase 2+3: combined cherry + merge-tree check (parallel) ──
# For each remaining branch:
#   - First try cheap cherry check (detects rebases)
#   - If that fails, try merge-tree (detects squash merges)
check_branch() {
  local branch="$1" base_branch="$2" base_tree="$3" merged="$4" unmerged="$5"

  # Cherry check: if no "+" lines, all commits have equivalents in base
  if ! git cherry "$base_branch" "$branch" 2>/dev/null | grep -q '^+'; then
    echo "$branch  (rebased)" >> "$merged"
    return
  fi

  # Tree check: simulate merge, see if it changes anything
  if merge_tree=$(git merge-tree --write-tree --no-messages "$base_branch" "$branch" 2>/dev/null); then
    if [[ "$merge_tree" == "$base_tree" ]]; then
      echo "$branch  (squash-merged)" >> "$merged"
      return
    fi
  fi

  echo "$branch" >> "$unmerged"
}
export -f check_branch

echo "  Phase 2+3 (cherry + squash): checking $remaining branches with $PARALLEL workers..." >&2

if [[ "$remaining" -gt 0 ]]; then
  cat "$phase1_remaining" | xargs -P "$PARALLEL" -I{} bash -c \
    'check_branch "$1" "$2" "$3" "$4" "$5"' _ {} "$BASE_BRANCH" "$BASE_TREE" "$merged_file" "$unmerged_file"
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
