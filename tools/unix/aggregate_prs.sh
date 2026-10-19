#!/usr/bin/env bash
set -euo pipefail

# Usage: ./aggregate_prs.sh 123 456 789
# This will create stacked branches: pr/123 -> pr/456 -> pr/789

REMOTE="origin"
BASE_BRANCH="master"

# Ensure we’re up to date
git fetch "$REMOTE"

# Start from base branch
git checkout -B aggregated-base "$REMOTE/$BASE_BRANCH"

# Process each PR number
for PR in "$@"; do
    BRANCH_NAME="pr/$PR"
    FETCH_REF="pull/$PR/head:$BRANCH_NAME"

    echo "=== Fetching PR #$PR from $REMOTE ==="
    git fetch "$REMOTE" "$FETCH_REF" --force

    echo "=== Rebasing $BRANCH_NAME on top of current branch ==="
    git checkout "$BRANCH_NAME"
    git rebase @{-1}  # rebase on the previous branch

    echo "=== Done: $BRANCH_NAME ==="
done

echo "✅ All PRs have been fetched and rebased in sequence."

