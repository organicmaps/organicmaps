# NonGPS Maps: how we work

This repo is a fork of Organic Maps. Two people build the navigation without GPS, each with their own Claude Code:

- **Ramzess** (`Ramzess-II`, repo owner): the core `libs/map/nogps`, its road search (`libs/routing/index_router.*`,
  `libs/map/routing_manager.*`), and `android/`. Windows: no desktop or iOS build; core tests run on an Android phone
  and in CI.
- **koloboid**: `iphone/`, `xcode/`. macOS: can build desktop targets and run `map_tests`.

Keep upstream files (`CLAUDE.md`, nested `CLAUDE.md`, `.claude/settings.json`, upstream skills) unchanged so upstream
merges stay clean. Fork rules go in `.claude/rules/nogps-*.md`, fork skills in `.claude/skills/nogps-*`.

## Flow of a change

1. **Idea → issue.** Run `/nogps-plan <idea>`. It checks open issues and PRs for overlap and writes the plan into
   the issue. Skip the plan when the diff fits in one sentence.
2. **Implement in a fresh session** (`/clear`, then "implement #N") on `feature/<name>` from current `origin/master`.
3. **Finish with `/nogps-pr`**: tests, living docs, issue for the other platform, review in a fresh context, PR.
4. **Merge only through a PR.** When the repo variable `CLAUDE_REVIEW` is `on`, CI posts a Claude review when a PR
   opens. Comment `@claude <request>` on a PR to ask for a re-review or a fix. If the PR changes the shared
   interface or the other person's platform, the other person approves it. Otherwise the author merges after CI
   passes.

## Work you don't do now goes to an issue

IMPORTANT: anything you notice that this session won't finish goes into a GitHub issue before the session ends or
its PR opens: what the other platform must repeat or should get, a bug or risk you noticed, a TODO, a check on a
device, a step left for later. Not into docs, PR text, code comments or memory: nobody tracks those.

- Look for an open issue first (`gh issue list --state open`) and comment there instead of opening a duplicate.
- Label it `core`, `ios`, `android` or `firmware`, and assign the owner: `koloboid` for iOS, `Ramzess-II` for the
  rest. Say what to do and where, and which PR caused it.
- Link it from the PR's **Other platform** section. CI fails a PR whose **Other platform** section has neither an
  issue link nor "Nothing to do" and why.
- Give the user the links of the issues you opened.

## Shared interface

The platforms see the core through `libs/map/nogps/engine.hpp` (public methods, `Status`, `Event`), `delegate.hpp`,
and the order of values in their enums. The bridges are `android/sdk/src/main/cpp/app/organicmaps/sdk/NoGps.cpp` with
`NoGps.java`, and `iphone/Maps/Core/NoGps/MWMNoGps.h/.mm`. When the interface changes:

- open an issue labeled `ios` or `android` that says exactly what to repeat in the other bridge, and link it from
  the PR;
- a new file in `libs/map/nogps` goes into `libs/map/CMakeLists.txt` and `xcode/map/map.xcodeproj/project.pbxproj`.
  If the author can't build iOS, the PR says "Xcode project not verified".

## Living docs

Update these in the same PR, in a separate `[docs]` commit:

- `docs/nogps/navigation-rules.md`: the current rules of the core and their constants. Read the section you need
  before changing core behavior, not the whole file. If you change a rule or a threshold, change it here too.
- `docs/nogps/esp32-firmware-spec.md`: the box protocol and firmware.

Docs say what is true now. Why it got that way stays in commits and PRs: `git log -- <file>`, `gh pr view <n>`.
Work left to do lives in issues, not in markdown lists.

## Verification

- Core: unit tests `NoGps_*` in `map_tests`. macOS: `cmake --build build-<name> --target map_tests` then
  `build-<name>/map_tests --filter="NoGps_.*"`. Windows: the `C++ tests` CI check on the PR.
- A change to how the mark moves: replay the recorded trips with `tools/python/nogps_replay.py` (Android debug
  build) and put the before/after numbers in the PR. If nobody replayed it, the PR says so.
- Every behavior change gets a unit test.
- Strings: tag a string in `data/strings/strings.txt` only for the platforms whose code uses it. On iOS, write the
  key inside `L("key")`, not in a variable. Check with `python3 tools/python/validate_translation_keys.py`.
