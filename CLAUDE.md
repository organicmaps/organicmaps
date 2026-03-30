# Agent Instructions for Organic Maps

Organic Maps (OM) is an open-source, privacy-focused offline maps & GPS app built on OpenStreetMap data.
OM works on iOS, Android, and desktop (Windows, macOS, Linux using Qt 6).
The codebase is mostly C++23 with platform-specific code in Swift 5, Objective-C/C++, Java 17 and tools in bash and Python 3.10+

## Architecture overview

Libraries in `libs/` are strictly layered -- no circular or upward dependencies allowed:

1. **Foundation:** `base` (logging, assertions, caches), `coding` (serialization, I/O), `geometry` (points, rects, mercator)
2. **Platform:** `platform` (file paths, HTTP, location -- OS-abstracted via `*_ios.mm`, `*_android.cpp`, `*_qt.cpp`)
3. **Data:** `indexer` (map features, classification), `kml`, `transit`, `routing_common`, `editor`
4. **Domain:** `search`, `routing`, `storage` (map downloads), `traffic`
5. **Rendering:** `drape` (OpenGL/Vulkan/Metal abstraction), `drape_frontend` (scene management), `shaders`
6. **Application:** `map` (Framework class -- aggregates all subsystems)
7. **UI:** in the project's root: `qt/` (desktop), `iphone/` (iOS), `android/` (Android)

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
- Auto-format: `clang-format -i file.cpp` (v22+)
- Swift: format with `swiftformat iphone/` or `swiftformat <file>` (config in `iphone/.swiftformat`)
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

// All major types should implement DebugPrint():
std::string DebugPrint(MyType const & t);
```

## Environment setup
- Follow the instructions in [docs/INSTALL.md](docs/INSTALL.md)

## Build on desktop
1. CMake configure: `cmake -B build-$YOUR_NAME -S . -DCMAKE_BUILD_TYPE=Debug -GNinja` for debug configuration
   - if configure fails, repeat with `--fresh` option to clear CMake cache
2. Build: `cmake --build build-$YOUR_NAME` to build all targets
   or specify `--target target_name` to build a specific target (e.g., `desktop` for the main app)
3. To run tests:
```bash
# Exclude some tests that are not relevant for most contributors.
ctest -j --test-dir build-$YOUR_NAME --stop-on-failure --output-on-failure -E "drape_tests|generator_integration_tests|opening_hours_integration_tests|opening_hours_supported_features_tests|routing_benchmarks|routing_integration_tests|routing_quality_tests|search_quality_tests|storage_integration_tests|shaders_tests|world_feed_integration_tests"
# Run only a specific test:
ctest -j --test-dir build-$YOUR_NAME --stop-on-failure --output-on-failure -R test_name
```
4. To filter specific tests inside a test binary, use `--filter=<ECMA Regexp>` option.

### Common build targets
- `desktop` -- Qt desktop app
- `generator_tool` -- map generation CLI
- `dev_sandbox` -- developer debugging tool
- `<lib>_tests` -- test binary for a library (e.g., `base_tests`, `search_tests`, `routing_tests`)
- `skin_generator_tool`, `track_generator_tool`, `topography_generator_tool` -- auxiliary tools

## Build for iOS
```
xcodebuild archive -workspace xcode/omim.xcworkspace -configuration Debug -destination generic/platform='iOS Simulator' \
    -scheme OMaps MARKETING_VERSION="$(date +%Y.%m.%d)" CURRENT_PROJECT_VERSION=1
```

## Build for Android
- `cd android && ./gradlew assembleGoogleDebug -Parm64`

## Commit messages
Format: `[subsystem] Summary in imperative mood` (max 80 chars). Examples of subsystems:
`[android]`, `[ios]`, `[qt]`, `[search]`, `[routing]`, `[generator]`, `[strings]`, `[styles]`, `[platform]`, `[storage]`, `[bookmarks]`, `[3party]`, `[docs]`

- Separate subject from body with a blank line; wrap body at 80 chars
- Explain **what and why**, not how
- Link issues on last lines: `Fixes: #123`, `Closes: #456`
- Auto-generated files (strings, styles) must be in a **separate commit** with title like `[strings] Regenerated` or `[styles] Regenerated`
- Signed-off-by line required (DCO): `git commit -s`
- See [docs/COMMIT_MESSAGES.md](docs/COMMIT_MESSAGES.md) for full guide

## Pull requests
- Prefer PRs focused and small; split unrelated changes into separate PRs, or at least separate commits with clear messages
- Mention if LLM tools were used to generate code
- Description must include: what changed, link to issue (`Fixes #NNN`), how it was tested
- Every commit must compile on all platforms and pass tests
- New features require tests in the same PR
- Test on multiple OS versions, themes (light/dark), orientations where applicable
- See [docs/PR_GUIDE.md](docs/PR_GUIDE.md) for full guide

## Main focus
- On performance, simplicity, regressions, impact and code quality
- Unit tests covering most/all corner cases or changes
- Less code/cleaner code/less changes

## Code review guidelines
- Use `gh` CLI tool to review pull requests, leave comments and approve changes
- Check the code locally if the same PR/branch is checked out
- Review code for bugs, correctness, maintainability, performance, and other issues
- Review related changes and places where the code is used
- Review documentation and comments for clarity and accuracy
- Propose unit tests if needed/relevant
- Keep the architecture, design and implementation simple and clean, avoid overengineering and unnecessary complexity
- Verify no new circular dependencies between library layers
- Check that platform-specific code uses proper CMake conditionals or platform-suffixed files
- Verify auto-generated files are NOT modified by hand (styles, localizations, `classificator.txt`)
- Check that new strings go into translation files, not hardcoded
- Verify new map features have classifier tests in `generator/generator_tests/osm_type_test.cpp`

## Translation, map styles, and new map features workflows
See [data/CLAUDE.md](data/CLAUDE.md) for:
- Translation workflow (editing `data/strings/`, regenerating)
- Map styles workflow (editing MapCSS in `data/styles/`, regenerating)
- Adding a new map feature / search category (full checklist)
- Translation glossary and typography rules for Russian, Ukrainian, Belarusian

## Debug commands
Enter in the search bar to activate (see [docs/DEBUG_COMMANDS.md](docs/DEBUG_COMMANDS.md)):
- `?dark`/`?light`/`?olight`/`?odark` -- switch map themes
- `?debug-info` -- show zoom, FPS, renderer info
- `?debug-rect` -- show icon/label bounding boxes
- `?edits` -- list local map edits
- `?gl`/`?vulkan`/`?metal` -- force graphics backend (requires restart)

## Code ownership
See `.github/CODEOWNERS` for team assignments. Key teams:
`@organicmaps/android`, `@organicmaps/ios`, `@organicmaps/qt`, `@organicmaps/rendering`, `@organicmaps/data`, `@organicmaps/styles`

