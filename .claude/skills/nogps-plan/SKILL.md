---
name: nogps-plan
description: Turn an idea or bug for NoGPS Maps into a GitHub issue with a short plan before any code is written. Use when the user brings a new idea, feature, or bug to start on, or asks to plan issue #N.
argument-hint: "<idea in a few words> | <issue number>"
---

# Plan a NoGPS change

Input: an idea in words, or an existing issue number. Research only: don't change code in this skill.

1. **Overlap.** `gh issue list --state open --limit 50` and `gh pr list --state open`. If someone else's open issue
   or PR covers the same code or behavior, stop and show it to the user. Starting a second one wastes both people's
   time.
2. **Context.** For an issue number, `gh issue view <n> --comments`. Read the matching section of
   `docs/nogps/navigation-rules.md` and the code it points to. For searches across many files, use an Explore
   subagent. Ask the user only what the code and docs can't answer: expected behavior, which trip shows the problem,
   and trade-offs.
3. **Size.** If the change fits in one sentence, say so and suggest going straight to a branch. Write no plan.
4. **Plan.** Keep it to 40 lines or fewer, in English:
   - **Problem**: what goes wrong, with evidence (trip date and time, log line, numbers).
   - **Behavior after**: the rule as it will read in `navigation-rules.md`.
   - **Changes**: files and functions; new constants with their values.
   - **Shared interface**: changes to `engine.hpp`, `delegate.hpp`, enums, and what the other platform must repeat,
     or "none".
   - **Verification**: the unit tests to add, and which trips to replay with which numbers to compare.
   - **Out of scope**.
5. **Publish.** The repo is public. Show the issue text to the user and create it only after they confirm:
   `gh issue create --title "[<area>] <summary>" --label <core|ios|android|firmware> --assignee @me --body-file -`.
   For an existing issue, add the plan with `gh issue comment <n> --body-file -`.
6. **Hand off.** Tell the user to start a fresh session (`/clear`) and say "implement #<n>" on a new
   `feature/<name>` branch from `origin/master`.
