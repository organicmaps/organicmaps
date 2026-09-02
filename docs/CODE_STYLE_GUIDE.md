# Code Style Guide

Read the code, and follow the existing style as much as possible.

## ClangFormat

For C++, Java and Objective C/C++ code we use [clang-format](http://clang.llvm.org/docs/ClangFormat.html) version 23.

### Installation

- macOS: `brew install clang-format`
- Windows (MSYS2): `pacman -S clang`
- Ubuntu 24:
   ```bash
   wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
   echo 'deb http://apt.llvm.org/noble/ llvm-toolchain-noble-23 main' | sudo tee /etc/apt/sources.list.d/llvm-toolchain-noble-23.list
   sudo apt-get update
   sudo apt-get install -y clang-format-23  # Run it as clang-format-23
   ```
- Any OS, pinned to an exact version: `pip install clang-format==23.1.0`

Make sure that clang-format 23.x is in your PATH. The formatting scripts reject other major versions to prevent
formatting differences between local development and CI. Homebrew and MSYS2 track the current LLVM release, so
use the pinned `pip` package (or `brew install llvm@23`, binary in `$(brew --prefix llvm@23)/bin`) once they
move ahead of 23.

### Usage

- Configuration is in `.clang-format`
- Set up a `git commit` hook (see below) for automatic formatting of changed files
- To manually format a file run `clang-format -i file` (`clang-format-23` for Ubuntu)
- To format all files in the repository run `tools/unix/clang-format.sh`
- To format changes added to commit run `git clang-format`
- To format already committed changes run `git clang-format HEAD~1`

`git clang-format` runs whichever `clang-format` is first in PATH and does not check its version, unlike
`tools/unix/clang-format.sh` and the commit hook. Pass `--binary clang-format-23` if you have several versions
installed.

### ClangFormat in Android Studio

1. Install this plugin: https://plugins.jetbrains.com/plugin/20785-clang-format-tools
2. Then go to Preferences -> Other Settings -> ClangFormat and set the path to your installed `clang-format` binary.
3. Enable "Reformat code on file save" option.

### ClangFormat in VSCode

Install and setup the [Clang-Format extension](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format).

## Swift Style

We are using [swiftformat](https://github.com/nicklockwood/SwiftFormat) for Swift code. Install it and configure
format on save in Xcode by following instructions at https://medium.com/@jozott/format-on-save-xcode-swift-8133d049b3ac

### Installation

- macOS: `brew install swiftformat`

Make sure that swiftformat is in your PATH.

### Usage

- Configuration is in `iphone/.swiftformat`
- Run `swiftformat <somefile.swift>` to format a single file
- Set up a `git commit` hook (see below) for automatic formatting of changed files

## Kotlin Style

We are using [ktlint](https://pinterest.github.io/ktlint/) for Kotlin code formatting.

### Installation

The canonical ktlint version is pinned in `android/gradle/libs.versions.toml`
(field `ktlint`). CI installs that exact version; please match it locally to
avoid formatting drift between your machine and the `code-style-check-kotlin`
job.

- macOS: `brew install ktlint` (then verify `ktlint --version` matches the pinned version)
- Other platforms: download the pinned version from [GitHub Releases](https://github.com/pinterest/ktlint/releases)

Make sure that ktlint is in your PATH.

### Usage

- Configuration is in `android/.editorconfig`
- Run `ktlint --format <somefile.kt>` to format a single file
- To format all Kotlin files in the repository run `tools/unix/ktlint_format.sh`
- Set up a `git commit` hook (see below) for automatic formatting of changed files

## Kotlin Static Analysis (detekt)

We use [detekt](https://detekt.dev/) for Kotlin static analysis: naming conventions, complexity, code smells, and potential bugs.

### Installation

The canonical detekt version is pinned in `android/gradle/libs.versions.toml`
(field `detekt`). CI installs that exact version; please match it locally.

- Download the matching version from [GitHub Releases](https://github.com/detekt/detekt/releases) (`detekt-cli-<version>-all.jar`)
- Or run via Gradle (no local install needed): `cd android && ./gradlew :app:detektCheck`

### Usage

- Configuration is in `android/detekt.yml`
- Run `cd android && ./gradlew :app:detektCheck` to check the `app` module
- Run `cd android && ./gradlew detektCheck` to check all Kotlin-enabled modules

### Key rules

- Hungarian notation (`m` prefix) is forbidden for private properties: use `_camelCase` for backing fields, `camelCase` for regular properties
- Formatting rules are handled by ktlint, not detekt (no overlap)

## JSON and XML Style

Hand-edited JSON and XML files are formatted by
`tools/python/format_json_xml.py`. It uses only the Python 3 standard library,
so no additional formatter needs to be installed.

### Usage

- To check selected files, run
  `tools/python/format_json_xml.py files <file.json> <file.xml>`
- To format selected files, add `--fix` to that command
- To check all in-scope files, run `tools/python/format_json_xml.py all`
- To format all in-scope files, run
  `tools/python/format_json_xml.py all --fix`

The include and exclude rules are defined in `tools/python/format_json_xml.py`.
CI checks in-scope JSON and XML files touched by a pull request. The formatter
keeps an XML element on one line when it has at most one attribute, and
otherwise puts the tag name alone on the opening line with every attribute on
its own line -- except a leading `xmlns` declaration, which stays on the tag
line. It rejects duplicate or lossy JSON, and leaves XML that cannot be safely
reflowed untouched.

## Python Style

Follow the existing style in Python files as much as possible. We'll add a more detailed guide later.

## Automated formatting on pre-commit hook

Run `git config core.hooksPath tools/hooks` to set up the pre-commit hook.

After that, every time you commit, the hook will automatically format your
`.java`, `.kt`, `.swift`, `.cpp`, `.hpp`, `.m`, `.mm`, `.h`, `.cc`, `.json`,
and `.xml` files according to the project's style rules. JSON and XML include
and exclude rules are applied before formatting.

You can bypass the auto-formatting with `git commit --no-verify` if necessary.

To configure the formatting style, edit `.clang-format`, `.swiftformat` in the
project root, and `android/.editorconfig` for Kotlin.

To configure which source files are formatted, edit
`tools/hooks/format-config.bash`. JSON and XML scope is configured in
`tools/python/format_json_xml.py`.

## Tips and Hints

- Check the existing code base for examples of how to do things, and to reuse existing utilities and functions
- Write the code without warnings
- If you see outdated code which can be improved, DO IT NOW (but in a separate pull request or commit)!
- Your code should work at least on [mac|linux|android][x86|x86_64], [ios|android][x86|armv7|arm64] architectures
- Your C++ code should compile with C++23 compiler
- Avoid using any new 3party library if it is not fully tested and supported on all our platforms
- Cover your code with unit tests. See examples for existing libraries
- Ask if you have any questions
- If you don't have enough time to make it right, or see a potential issue, leave a `// TODO(DeveloperInitialsOrNickname): need to fix it` comment
- Use brief comments only for unobvious changes, describing rationale that is not immediately clear from the code

### Useful links

- [Google's coding standard](https://google.github.io/styleguide/cppguide.html)
- [raywenderlich.com Objective-C Style Guide](https://github.com/kodecocodes/objective-c-style-guide)
- [The Objective-C Programming Language](http://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/ObjectiveC/Introduction/introObjectiveC.html)
- [Cocoa Fundamentals Guide](https://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/CocoaFundamentals/Introduction/Introduction.html)
- [Coding Guidelines for Cocoa](https://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/CodingGuidelines/CodingGuidelines.html)
- [iOS App Programming Guide](http://developer.apple.com/library/ios/#documentation/iphone/conceptual/iphoneosprogrammingguide/Introduction/Introduction.html)
