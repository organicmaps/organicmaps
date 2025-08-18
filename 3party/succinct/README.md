succinct
========

This library contains the implementation of some succinct data
structures. It is rather undocumented now, but better documentation is
under way. On the other hand, the code is quite extensively
unit-tested.

The library is meant to be imported as a git submodule in other
projects and then included as a CMake subdirectory. See the unit
tests, and the [semi_index](https://github.com/ot/semi_index) and
[path_decomposed_tries](https://github.com/ot/path_decomposed_tries)
projects for examples.

How to build the code
---------------------

### Dependencies ###

The following dependencies have to be installed to compile the library.

* CMake >= 2.6, for the build system
* Boost >= 1.42

### Supported systems ###

The library is developed and tested mainly on Linux and Mac OS X, and
it has been tested also on Windows 7.

The code is designed for 64-bit architectures. It has been tested on
32-bit Linux as well, but it is significantly slower. To compile the
library on 32-bit architectures it is necessary to disable intrinsics
support, passing -DSUCCINCT_USE_INTRINSICS=OFF to cmake.

### Building on Unix ###

The project uses CMake. To build it on Unix systems it should be
sufficient to do the following:

    $ cmake .
    $ make

It is also advised to perform a `make test`, which runs the unit
tests.

### Builing on Mac OS X ###

Same instructions for Unix apply, with one exception: the library must
be compiled with the same standard library used to compile Boost. So,
if libc++ was used with Clang, the following command must be used:

    $ cmake . -DSUCCINCT_USE_LIBCXX=ON


### Building on Windows ###

On Windows, Boost and zlib are not installed in default locations, so
it is necessary to set some environment variables to allow the build
system to find them.

* For Boost `BOOST_ROOT` must be set to the directory which contains
  the `boost` include directory.
* The directories that contain the Boost must be added to `PATH` so
  that the executables find them

Once the env variables are set, the quickest way to build the code is
by using NMake (instead of the default Visual Studio). Run the
following commands in a Visual Studio x64 Command Prompt:

    $ cmake -G "NMake Makefiles" .
    $ nmake
    $ nmake test
