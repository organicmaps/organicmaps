# Agent Instructions for Organic Maps

Organic Maps (OM) is a privacy-focused offline maps & GPS app built on OpenStreetMap data.
OM works on iOS, Android, and desktop (Windows, macOS, Linux using Qt 6).
The codebase is mostly C++23 with some platform-specific code in Swift 5, Objective-C/C++, Java 17 and tools in Python 3.10+

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
- Auto-format: `clang-format -i file.cpp`
- Swift: format with `swiftformat iphone/` (argument is path to directory with Swift code)
- See [docs/CODE_STYLE_GUIDE.md](docs/CODE_STYLE_GUIDE.md) for more details and examples

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

## Build for iOS
```
xcodebuild archive -workspace xcode/omim.xcworkspace -configuration Debug -destination generic/platform='iOS Simulator' \
    -scheme OMaps MARKETING_VERSION="$(date +%Y.%m.%d)" CURRENT_PROJECT_VERSION=1
```

## Build for Android
- `cd android && ./gradlew assembleGoogleDebug -Parm64`

## Main focus
- On performance and code quality
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

## When translating content from English:
- "bookmark" or "bookmarks" (favorite place, saved by user) , use word "метка" or "метки" for Russian, use word "мітка" or "міткі" for Ukrainian
- "track" (recorded path on the map that user walked) to Russian, use word "трек"
- "route" to Russian, use word "маршрут"
- "icon" or "icons" (place's image symbol on the map), use word "иконка" или "иконки" for Russian
- "outdoors" or "outdoors style" or "outdoors map style" use "стиль для активного отдыха" for Russian, use "режим Активний відпочинок" for Ukrainian.
- "map" use "мапа" for Ukrainian and Belarusian.
- use "…" instead of "..."
- use ё instead е in Russian where applicable
- do not translate "Organic Maps" and "ID Editor"
- do not replace amounts like 5K with zeroes (5.000), either leave it (if it is a normal language practice) or use "5 thousands" equivalent (like 5 тыс. in Russian)
