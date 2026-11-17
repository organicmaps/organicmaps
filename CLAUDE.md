# Agent Instructions for Organic Maps

Organic Maps (OM) is an open-source, privacy-focused offline maps & GPS app built on OpenStreetMap data.
OM works on iOS, Android, and desktop (Windows, macOS, Linux using Qt 6).
The codebase is mostly C++23 with platform-specific code in Swift 5, Objective-C/C++, Java 17 and tools in bash and Python 3.11+

## Architecture overview

Libraries in `libs/` are strictly layered -- no circular or upward dependencies allowed:

1. **Foundation:** `base` (logging, assertions, caches), `coding` (serialization, I/O), `geometry` (points, rects, mercator)
2. **Platform:** `platform` (file paths, HTTP, location -- OS-abstracted via `*_ios.mm`, `*_android.cpp`, `*_qt.cpp`)
3. **Data:** `indexer` (map features, classification), `terrain` (`.twm` terrain files, contours), `kml`, `transit`, `routing_common`, `editor`
4. **Domain:** `search`, `routing`, `storage` (map downloads), `traffic`
5. **Rendering:** `drape` (OpenGL/Vulkan/Metal abstraction), `drape_frontend` (scene management), `shaders`
6. **Application:** `map` (Framework class -- aggregates all subsystems)
7. **UI:** in the project's root: `qt/` (desktop), `iphone/` (iOS), `android/` (Android), `dev_sandbox/` (graphics dev tool)

Key namespaces: `m2::` (2D geometry, e.g. `m2::PointD`, `m2::RectD`), `ms::LatLon` (WGS84), `mercator::` (coordinate conversion), `search::`, `routing::`, `storage::`, `kml::`

The C++ core is accessed from platforms via bridging layers:
- iOS: `iphone/CoreApi/` framework (Objective-C++ wrappers) -> Swift via `Bridging-Header.h`
- Android: JNI in `android/sdk/src/main/cpp/` -> Java `Framework.java` / `OrganicMaps.java`
- See [docs/STRUCTURE.md](docs/STRUCTURE.md) for directory layout

## Code Style
- C++ and Java format rules described in `.clang-format`
- `.hpp/.cpp` files (not `.h/.cc`)
- `#pragma once` for headers
- 2-space indent, 120 char line width
- Member variables: `m_` prefix (e.g., `m_countryFile`)
- `const` after the type: `auto const & ref = f();`
- Namespaces: `lower_case` with underscores
- `using` instead of `typedef`
- Compile-time constants: `kCamelCase` and `constexpr`
- Comments should be brief, explaining only reasoning that is not obvious from the code itself
- Auto-format: `clang-format -i file.cpp` (v23), or `tools/unix/clang-format.sh` for the whole tree
- Swift: format with `swiftformat iphone/` or `swiftformat <file>` (config in `iphone/.swiftformat`)
- Kotlin: format with `tools/unix/ktlint_format.sh` (config in `android/.editorconfig`)
- Style checks gate CI (clang-format, swiftformat, ktlint, and detekt static analysis) -- run them before pushing
- Pre-commit hook (auto-formats on commit): `git config core.hooksPath tools/hooks`
- See [docs/CODE_STYLE_GUIDE.md](docs/CODE_STYLE_GUIDE.md) for more details and examples

### C++ include conventions
```cpp
#pragma once

#include "geometry/point2d.hpp"   // Project includes: relative to repo root, in quotes
#include "base/assert.hpp"

#include <vector>                 // Standard library: in angle brackets
#include <string>

#include <boost/noncopyable.hpp>  // Third-party: in angle brackets
```

### C++ testing patterns
```cpp
namespace test_file_name
{
// Unit tests use custom macros from testing/testing.hpp, not Google Test:
UNIT_TEST(MyTestName)
{
  TEST(condition, ("message", extra_context));
  TEST_EQUAL(a, b, ());
  TEST_NOT_EQUAL(a, b, ());
  TEST_LESS(a, b, ());
  TEST_GREATER(a, b, ());
  TEST_ALMOST_EQUAL_ABS(a, b, eps, ());
}
// Test files go in libs/<module>/<module>_tests/<name>_tests.cpp
// Register tests in the corresponding CMakeLists.txt using omim_add_test()
}  // namespace test_file_name
```

### C++ assertions, logging and exceptions
```cpp
// Assertions (base/assert.hpp) -- ASSERT is debug-only, CHECK is always-on:
CHECK(condition, ("context", value));
CHECK_EQUAL(a, b, ());
ASSERT(condition, ("debug-only check"));

// Logging (base/logging.hpp):
LOG(LINFO, ("message", value));
LOG(LWARNING, ("warning"));
LOG(LERROR, ("error"));

// Exceptions (base/exception.hpp):
DECLARE_EXCEPTION(MyException, RootException);
MYTHROW(MyException, ("Cannot process", filename));

// All major types used in LOG and CHECK/ASSERT should implement DebugPrint():
std::string DebugPrint(MyType const & t);
```

## Environment setup
- Follow the instructions in [docs/INSTALL.md](docs/INSTALL.md)

## Build on desktop
1. CMake configure: `cmake --preset debug -B build-$YOUR_NAME` for debug configuration
   - the `debug` preset (in `CMakePresets.json`) sets Ninja + `CMAKE_BUILD_TYPE=Debug`; `-B` overrides the preset's build dir so each agent gets its own `build-$YOUR_NAME` and they don't clobber each other
   - `$YOUR_NAME` is a placeholder, not a shell variable: use your agent's name (e.g. `build-claude`) and reuse the dir if it already exists
   - if configure fails, repeat with `--fresh` option to clear CMake cache
2. Build: `cmake --build build-$YOUR_NAME` to build all targets
   or specify `--target target_name` to build a specific target (e.g., `desktop` for the main app)
3. To run tests:
```bash
# Mirrors the default CMake test preset while keeping the per-agent build dir.
CTEST_EXCLUDE_REGEX="drape_tests|drape_frontend_tests|generator_integration_tests|routing_benchmarks|routing_integration_tests|routing_quality_tests|search_quality_tests|storage_integration_tests|shaders_tests"
ctest -j --test-dir build-$YOUR_NAME --stop-on-failure --output-on-failure -L "omim-test" -E "$CTEST_EXCLUDE_REGEX"
# Rendering tests need offscreen GL and are run separately.
QT_QPA_PLATFORM=offscreen ctest --test-dir build-$YOUR_NAME --stop-on-failure --output-on-failure -R "drape_tests|drape_frontend_tests|shaders_tests"
# Run only a specific test:
ctest -j --test-dir build-$YOUR_NAME --stop-on-failure --output-on-failure -R test_name
```
4. To filter specific tests inside a test binary, use `--filter=<ECMA Regexp>` option.
5. Always check return result when launching test binaries to detect crashed/segfaulted test. Use `${pipestatus[*]}` array for zsh (default on MacOS) and `${PIPESTATUS[*]}` for bash.

### Common build targets
- `desktop` -- Qt desktop app
- `generator_tool` -- map generation CLI
- `dev_sandbox` -- developer graphics engine debugging tool
- `<lib>_tests` -- test binary for a library (e.g., `base_tests`, `search_tests`, `routing_tests`)
- `skin_generator_tool`, `track_generator_tool`, `topography_generator_tool` -- auxiliary tools

## Mobile builds and CI
- Read the [Android guide](android/CLAUDE.md) for Android work, including CI. For changes to the
  SDK or its CI, also read the [SDK guide](android/sdk/CLAUDE.md).
- Read the [iOS guide](iphone/CLAUDE.md) for iOS builds and changes to `iphone/`, `xcode/`, or iOS CI.

## Commit messages
Format: `[subsystem] Summary in imperative mood` (max 80 chars). Examples of subsystems:
`[android]`, `[ios]`, `[qt]`, `[search]`, `[routing]`, `[generator]`, `[strings]`, `[styles]`, `[platform]`, `[storage]`, `[bookmarks]`, `[3party]`, `[docs]`

- Separate subject from body with a blank line; wrap body at 80 chars
- Explain **what and why**, not how; keep it brief
- Link issues on last lines: `Fixes: #123`, `Closes: #456`
- Auto-generated files (strings, styles) must be in a **separate commit** with title like `[strings] Regenerated` or `[styles] Regenerated`
- Signed-off-by line required (DCO): `git commit -s`
- See [docs/COMMIT_MESSAGES.md](docs/COMMIT_MESSAGES.md) for full guide

## Pull requests
- Prefer PRs focused and small; split unrelated changes into separate PRs, or at least separate commits with clear messages
- Mention if LLM tools were used to generate code, include model name and effort level
- Description must be brief (don't wrap markdown) and include: what changed, link to issue (`Fixes #NNN` or `Related #NNN`), how it was tested
- Label the PR properly if permissions allow it
- Every commit must compile on all platforms and pass tests; exceptions are possible for easier code review, if PR will be squashed, or for android using string ids before strings regenerated commit
- New features require tests in the same PR
- Test on multiple OS versions, themes (light/dark), orientations where applicable
- See [docs/PR_GUIDE.md](docs/PR_GUIDE.md) for full guide

## Main focus
- On performance, simplicity, regressions, impact and code quality
- Tests covering supported behavior, reachable boundary failures and actual regressions
- Less code/cleaner code/less changes
- Simple architecture and design for long-term maintenance

## Error handling policy
- Validate user input and untrusted external data once at the owning boundary. Handle expected
  environmental failures and cancellations normally. Inspect producer guarantees before adding
  checks, and rely on established preconditions, ownership, lifetimes and state invariants downstream.
- Fix internal contract violations at their source, including those exposed by valid user actions.
  Reserve recovery paths for expected failures.
- Use `ASSERT` for useful Debug diagnostics on non-obvious invariants or bugs under investigation.
  Use `CHECK` only when a concrete requirement calls for Release enforcement.
- Keep `ASSERT` expressions side-effect free because Release omits them. Perform required work,
  including reads and decompression, outside `ASSERT` or use `VERIFY`.
- Use `math::is_finite` when a finiteness check is required under fast-math. Successful
  `strings::to_double` and `strings::to_float` calls guarantee finite results. Recheck only if a
  subsequent operation can invalidate that guarantee.
- Verify download hashes when available and trust downloaded files from OM servers. Limit on-disk
  corruption recovery to startup registration. `Framework::RegisterAllMaps()` must log and skip
  broken map files and invalid terrain blocks so the app can start and export bookmarks. Keep Debug
  assertions for terrain invariants.
- Use `MYTHROW` for file format or constraint failures during startup registration, such as
  unsupported versions, so registration can log and skip the file. After registration accepts a
  file, use `CHECK` in once-per-file header or prologue loading and at section offsets and lengths
  when Release enforcement is needed to prevent out-of-bounds reads or excessive allocations.
  Include the value and bound in the message.
- Use Debug-only `ASSERT` for other invariant checks in regular map and terrain tile records and
  payloads, including stream-derived coordinates, indices and counts decoded per element.

## Code review guidelines

For code reviews, read and follow the [om-review skill](.github/skills/om-review.md).

## Translation, map styles, and new map features workflows

Read the [data guide](data/CLAUDE.md) for translations (including store metadata), map styles,
new map features, or search categories.

## Debug commands

See the [debug command guide](docs/DEBUG_COMMANDS.md) for commands you can enter in the search bar.

## Code ownership
See `.github/CODEOWNERS` for team assignments. Key teams:
`@organicmaps/android`, `@organicmaps/ios`, `@organicmaps/qt`, `@organicmaps/rendering`, `@organicmaps/data`, `@organicmaps/styles`
