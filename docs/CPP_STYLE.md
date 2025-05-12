# C++ Style Guide

In general, [Google's coding standard](https://google.github.io/styleguide/cppguide.html) is used, and we strongly encourage to read it.

Below are our specific (but not all!) exceptions to the Google's coding standard:

- All C++ code should conform to the C++20 standard.
- We use `.cpp` and `.hpp` files, not `.cc` and `.h` (`.c` and `.h` are used for C code), in UTF-8 encoding.
- File names are lowercase with underscores, like `file_reader.cpp`.
- We use `#pragma once` instead of the `#define` Guard in header files.
- Includes are sorted and grouped by directory, there should be newlines between different directories.
- Order of directories in includes: "current_dir/current_file.hpp", includes from other dirs sorted by dependencies (e.g. indexer, then coding, then base), "defines.hpp", C++ standard library headers, boost headers, 3party.
- We ARE using C++ exceptions.
- We are using all features of C++17 and C++20 except std::filesystem, std::to_chars & std::from_chars which are not fully supported on all platforms.
- We try to limit the usage of boost libraries which require linking (and prefer C++20 types over their boost counterparts).

Naming and formatting

- We ALWAYS use two spaces indent and don't use tabs.
- We don't have strict limits on line width, but keep it reasonable to fit on the screen. The advised width is that written in the [.clang-format](.clang-format) file (currently 100).
- Doxygen-style comments can be used.
- Underscores are allowed only in prefixes for member variables and namespace names, like `int m_countriesCount; namespace utf_parser`.
- Use right-to-left order for variables/params: `string const & s` (reference to the const string).
- In one line `if`, `for`, `while` we do not use brackets. If one line `for` or `while` is combined with one line `if`, do use brackets for cycle.
- Space after the keyword in conditions and loops. Space after `;` in `for` loop.
- Space between binary operators: `x = y * y + z * z`.
- Space after double dash.
- We use `using` keyword instead of `typedef`.
- We do not use the Systems Hungarian Notation: do not add the "p" suffix to your pointer variable names and the "T" prefix or suffix to your type names.
- Compile-time constants must be named in CamelCase, starting with a lower-case `k`, e.g. `kCompileTimeConstant` and marked as `constexpr` when possible.
- Values of enum classes must be named in CamelCase, e.g. `enum class Color { Red, Green, LightBlue };`.
- Macros and C-style enums must be named in UPPER_CASE, and enum values must be prefixed with a capitalized enum name.

  Note that macros complicate debugging, and old-style enums have dangerous implicit conversions to integers, and tend to clutter
  containing namespaces. Avoid them when possible - use `const` or `constexpr` instead of macros, and enum classes instead of enums.

**We write code without warnings!**

## ClangFormat

Most of our coding style is specified in a configuration file for [ClangFormat](http://clang.llvm.org/docs/ClangFormat.html).
To automatically format a file, install `clang-format` and run:

    clang-format -i file.cpp file.hpp other_file.cpp

## Formatting Example/Guide/Reference

```cpp
#pragma once

#include <math>

uint16_t constexpr kBufferSize = 255;

// C-style enums are ALL_CAPS. But remember that C++11 enum classes are preferred.
enum Type
{
  TYPE_INTEGER,
  TYPE_FLOAT,
  TYPE_STRING
};

using TMyTypeStartsWithCapitalTLetter = double;

class ComplexClass
{
public:
  Complex(double rePart, double imPart) : m_re(rePart), m_im(imPart) {}

  double Modulus() const
  {
    double const rere = m_re * m_re;
    double const imim = m_im * m_im;
    return sqrt(rere + imim);
  }

  double OneLineMethod() const { return m_re; }

private:
  // We use the "m_" prefix for member variables.
  double m_re;
  double m_im;
};

namespace
{
void CamelCaseFunctionName(int lowerCamelCaseVar)
{
  static int counter = 0;
  counter += lowerCamelCaseVar;
}
}  // namespace

namespace lower_case
{
template <typename TypenameWithoutAffixes>
void SomeFoo(int a, int b,
             TypenameWithoutAffixes /* We avoid compilation warnings. */)
{
  for (int i = 0; i < a; ++i)
  {
    // IMPORTANT! We DON'T use one-liners for if statements for easier debugging.
    // The following syntax is invalid: if (i < b) Bar(i);
    if (i < b)
      Bar(i);
    else
    {
      Bar(i);
      Bar(b);
      // Commented out the call.
      // Bar(c);
    }
  }
}
}  // namespace lower_case

// Switch formatting.
int Foo(int a)
{
  switch (a)
  {
  case 1:
    Bar(1);
    break;
  case 2:
  {
    Bar(2);
    break;
  }
  case 3:
  default:
    Bar(3);
    break;
  }
  return 0;
}

// Loops formatting.

if (condition)
  foo();
else
  bar();

if (condition)
{
  if (condition)
    foo();
  else
    bar();
}

for (size_t i = 0; i < size; ++i)
  foo(i);

while (true)
{
  if (condition)
    break;
}

// Space after the keyword.
if (condition)
{
}

for (size_t i = 0; i < 5; ++i)
{
}

while (condition)
{
}

switch (i)
{
}

// Space between operators, and don't use space between unary operator and expression.
x = 0;
x = -5;
++x;
x--;
x *= 5;
if (x && !y)
{
}
v = w * x + y / z;
v = w * (x + z);

// Space after double dash. And full sentences in comments.
```

## Tips and Hints

- If you see outdated code which can be improved, DO IT NOW (but in a separate pull request or commit)!
- Your code should work at least on [mac|linux|android][x86|x86_64], [ios|android][x86|armv7|arm64] architectures
- Your code should compile with C++20 compiler
- Avoid using any new 3party library if it is not fully tested and supported on all our platforms
- Cover your code with unit tests. See examples for existing libraries
- Check Base and Coding libraries for most of the basic functions
- Ask your team if you have any questions
- Use dev@comaps.app mailing list to ask all developers and bugs@comaps.app mailing list to post bugs
- Release builds contain debugging information (for profiling), production builds do not
- If you don't have enough time to make it right, leave a `// TODO(DeveloperName): need to fix it` comment

### Some useful macros:

- `#ifdef DEBUG | RELEASE`
- `#ifdef OMIM_OS_ANDROID | OMIM_OS_IPHONE | OMIM_OS_MAC` (and some other useful OS-related macros, see `std/target_os.hpp`)
- Use `ASSERT(expression, (out message))` and `ASSERT_XXXXXX` macros often to check code validity in DEBUG builds
- Use `CHECK(expression, (out message))` and `CHECK_XXXXXX` macros to check code validity in all builds
- Use `LOG(level, (message))` for logging, below is more detailed description for level:
  - `LINFO` - always prints log message
  - `LDEBUG` - logs only in DEBUG
  - `LWARNING` - the same as `LINFO` but catches your attention
  - `LERROR` - the same as `LWARNING`, but crashes in DEBUG and works in RELEASE
  - `LCRITICAL` - the same as `LERROR` and ALWAYS crashes
- Need scope guard? Check `SCOPE_GUARD(name, func)`
- Use `std::array::size()` to calculate plain C-array's size
- Declare your own exceptions with `DECLARE_EXCEPTION(name, baseException)`, where `baseException` is usually `RootException`
- Throw exceptions with `MYTHROW(exceptionName, (message))`
- A lot of useful string conversion utils are in `base/string_utils.hpp`
