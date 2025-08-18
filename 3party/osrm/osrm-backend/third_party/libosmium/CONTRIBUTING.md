
# Notes for Developers

Read this if you want to contribute to Libosmium.


## Versioning

Osmium is currently considered in beta and doesn't use versioning yet. Proper
versions will be introduced as soon as it is somewhat stable.


## Namespace

All Osmium code MUST be in the `osmium` namespace or one of its sub-namespaces.


## Include-Only

Osmium is a include-only library. You can't compile the library itself. There
is no libosmium.so.

One drawback ist that you can't have static data in classes, because there
is no place to put this data.

All free functions must be declared `inline`.


## Coding Conventions

These coding conventions have been changing over time and some code is still
different.

* All include files have `#ifdef` guards around them, macros are the path name
  in all uppercase where the slashes (`/`) have been changed to underscore (`_`).
* Class names begin with uppercase chars and use CamelCase. Smaller helper
  classes are usually defined as struct and have lowercase names.
* Macros (and only macros) are all uppercase. Use macros sparingly, usually
  a constexpr is better.
* Variables, attributes, and function names are lowercase with
  `underscores_between_words`.
* Class attribute names start with `m_` (member).
* Template parameters are single uppercase letters or start with uppercase `T`
  and use CamelCase.
* Typedefs have `names_like_this_type` which end in `_type`.
* Macros should only be used for controlling which parts of the code should be
  included when compiling.
* Use `descriptive_variable_names`, exceptions are well-established conventions
  like `i` for a loop variable. Iterators are usually called `it`.
* Declare variables where they are first used (C++ style), not at the beginning
  of a function (old C style).
* Names from external namespaces (even `std`) are always mentioned explicitly.
  Do not use `using` (except for `std::swap`). This way we can't even by
  accident pollute the namespace of the code including Osmium.
* `#include` directives appear in three "blocks" after the copyright notice.
  The blocks are separated by blank lines. First block contains `#include`s for
  standard C/C++ includes, second block for any external libs used, third
  block for osmium internal includes. Within each block `#include`s are usually
  sorted by path name. All `#include`s use `<>` syntax not `""`.
* Names not to be used from outside the library should be in a namespace
  called `detail` under the namespace where they would otherwise appear. If
  whole include files are never meant to be included from outside they should
  be in a subdirectory called `detail`.
* All files have suffix `.hpp`.
* Closing } of all classes and namespaces should have a trailing comment
  with the name of the class/namespace.
* All constructors with one or more arguments should be declared "explicit"
  unless there is a reason for them not to be. Document that reason.

Keep to the indentation and other styles used in the code. Use `make indent`
in the toplevel directory to fix indentation and styling. It calls `astyle`
with the right parameters. This program is in the `astyle` Debian package.


## C++11

Osmium uses C++11 and you can use its features such as auto, lambdas,
threading, etc. There are a few features we do not use, because even modern
compilers don't support them yet. This list might change as we get more data
about which compilers support which feature and what operating system versions
or distributions have which versions of these compilers installed.

GCC 4.6   - too old, not supported (Ubuntu 12.04 LTS)
GCC 4.7.2 - can probably not be supported (Debian wheezy/stable)
GCC 4.7.3 - works
GCC 4.8   - works
clang 3.0 - too old, not supported (Debian wheezy/stable, Ubuntu 12.04 LTS)
clang 3.2 - works

C++11 features you should not use:
* Inherited Constructors (works only in GCC 4.8+ and clang 3.3+, not in Visual
  Studio)


## Checking your code

The Osmium makefiles use pretty draconian warning options for the compiler.
This is good. Code MUST never produce any warnings, even with those settings.
If absolutely necessary pragmas can be used to disable certain warnings in
specific areas of the code.

If the static code checker `cppcheck` is installed, the CMake configuration
will add a new build target `cppcheck` that will check all `.cpp` and `.hpp`
files. Cppcheck finds some bugs that gcc/clang doesn't. But take the result
with a grain of salt, it also sometimes produces wrong warnings.

Set `BUILD_HEADERS=ON` in the CMake config to enable compiling all include
files on their own to check whether dependencies are all okay. All include
files MUST include all other include files they depend on.

Call `cmake/iwyu.sh` to check for proper includes and forward declarations.
This uses the clang-based `include-what-you-use` program. Note that it does
produce some false reports and crashes often. The `osmium.imp` file can be
used to define mappings for iwyu. See the IWYU tool at
<http://code.google.com/p/include-what-you-use/>.


## Testing

There are a unit tests using the Catch Unit Test Framework in the `test`
directory and some data tests in `test/osm-testdata`. They are built by the
default cmake config. Run `ctest` to run them. Many more tests are needed.


## Documenting the code

All namespaces, classes, functions, attributes, etc. should be documented.

Osmium uses the Doxygen (www.doxygen.org) source code documentation system.
If it is installed, the CMake configuration will add a new build target, so
you can build it with `make doc`.

