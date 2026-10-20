---
name: nogps-pr
description: Finish a NoGPS change and open its pull request. Covers tests, living docs, an issue for the other platform, a review in a fresh context, and the PR description. Use when a change is ready for a PR.
argument-hint: "[issue number]"
---

# Finish a NoGPS change

1. **Scope.** `git fetch origin`, then review `git diff origin/master...HEAD` and `git status`. Commit only what
   belongs to this change. Local Xcode project noise and unrelated files stay out. If `origin/master` moved under
   the branch, tell the user before merging it in.
2. **Verify.** Follow "Verification" in `.claude/rules/nogps-workflow.md`. Paste the test command and its result.
   On Windows, say that CI runs the C++ tests. Don't claim tests passed if they didn't run.
3. **Living docs.** If a rule or constant of the core changed, update `docs/nogps/navigation-rules.md`. If the box
   protocol changed, update `docs/nogps/esp32-firmware-spec.md`. Put these in a separate `[docs]` commit.
4. **Other platform and follow-ups.** Open an issue now for everything this change leaves to do, as "Work you don't
   do now goes to an issue" in the rule says. That covers an interface change the other bridge must repeat, a
   feature the other platform should get, a bug noticed on the way, a check on a device. Generated strings go in
   their own `[strings] Regenerated` commit, as `CLAUDE.md` says.
5. **Review.** Run the `/code-review` skill on the branch. It works in a fresh context. Fix the correctness findings.
   Skip style preferences and defensive code for cases that can't happen.
6. **PR.** The repo is public. Show the title and body to the user, with the links of the issues from step 4, and
   create the PR only after they confirm. Push, then run `gh pr create --base master` with this body:
   - `Fixes #<issue>` when there is one;
   - **Why**: the problem and its evidence;
   - **What**: behavior changes, with names of constants and functions;
   - **Tested**: commands and results, and replay numbers before and after;
   - **Other platform**: the links of the issues from step 4, or "Nothing to do" and why. CI fails the PR without
     one of them. Add "Xcode project not verified" when that applies;
   - "Written with Claude Code."
7. **Reviewer.** If the PR touches the shared interface or the other person's platform, request the other person:
   `gh pr edit <n> --add-reviewer <Ramzess-II|koloboid>`.
