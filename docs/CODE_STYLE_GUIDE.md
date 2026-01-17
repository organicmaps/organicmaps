# Code Style Guide

Read the code, and follow the existing style as much as possible.

## ClangFormat

For C++, Java and Objective C/C++ code we use [clang-format](http://clang.llvm.org/docs/ClangFormat.html).

Install `clang-format` 21.0+ and make sure it's in your PATH.

- To manually format a file run `clang-format -i file`.
- To format all files in the repository run `tools/unix/clang-format.sh`
- To format changes added to commit run `git clang-format`
- To format already committed changes run `git clang-format HEAD~1`

### ClangFormat in Android Studio

1. Install this plugin: https://plugins.jetbrains.com/plugin/20785-clang-format-tools
2. Then go to Preferences -> Other Settings -> ClangFormat and set the path to your installed `clang-format` binary.
3. Enable "Reformat code on file save" option.

### ClangFormat in VSCode

Install and setup the [Clang-Format extension](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format).

## Swift Style

We are using [swiftformat](https://github.com/nicklockwood/SwiftFormat) for Swift code. Install it and configure
format on save in Xcode by following instructions at https://medium.com/@jozott/format-on-save-xcode-swift-8133d049b3ac

## Python Style

Follow the existing style in Python files as much as possible. We'll add a more detailed guide later.

## Tips and Hints

- Check the existing code base for examples of how to do things, and to reuse existing utilities and functions
- Write the code without warnings
- If you see outdated code which can be improved, DO IT NOW (but in a separate pull request or commit)!
- Your code should work at least on [mac|linux|android][x86|x86_64], [ios|android][x86|armv7|arm64] architectures
- Your C++ code should compile with C++23 compiler
- Avoid using any new 3party library if it is not fully tested and supported on all our platforms
- Cover your code with unit tests. See examples for existing libraries
- Ask if you have any questions
- If you don't have enough time to make it right, or see a potential issue, leave a `// TODO(DeveloperName): need to fix it` comment

### Useful links

- [Google's coding standard](https://google.github.io/styleguide/cppguide.html)
- [raywenderlich.com Objective-C Style Guide](https://github.com/kodecocodes/objective-c-style-guide)
- [The Objective-C Programming Language](http://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/ObjectiveC/Introduction/introObjectiveC.html)
- [Cocoa Fundamentals Guide](https://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/CocoaFundamentals/Introduction/Introduction.html)
- [Coding Guidelines for Cocoa](https://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/CodingGuidelines/CodingGuidelines.html)
- [iOS App Programming Guide](http://developer.apple.com/library/ios/#documentation/iphone/conceptual/iphoneosprogrammingguide/Introduction/Introduction.html)
