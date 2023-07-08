# Mapbox Variant

An alternative to `boost::variant` for C++11.

[![Build Status](https://secure.travis-ci.org/mapbox/variant.svg)](https://travis-ci.org/mapbox/variant)
[![Build status](https://ci.appveyor.com/api/projects/status/v9tatx21j1k0fcgy)](https://ci.appveyor.com/project/Mapbox/variant)
[![Coverage Status](https://coveralls.io/repos/mapbox/variant/badge.svg?branch=master)](https://coveralls.io/r/mapbox/variant?branch=master)

# Why use Mapbox Variant?

Mapbox variant has the same speedy performance of `boost::variant` but is faster to compile, results in smaller binaries, and has no dependencies.

For example on OS X 10.9 with clang++ and libc++:

Test | Mapbox Variant | Boost Variant
---- | -------------- | -------------
Size of pre-compiled header (release / debug) | 2.8/2.8 MB         | 12/15 MB
Size of simple program linking variant (release / debug)     | 8/24 K             | 12/40 K
Time to compile header     | 185 ms             |  675 ms


# Depends

 - Compiler supporting `-std=c++11`

Tested with

 - g++-4.7
 - g++-4.8
 - clang++-3.4
 - clang++-3.5
 - Visual C++ Compiler November 2013 CTP
 - Visual C++ Compiler 2014 CTP 4

Note: get the "2013 Nov CTP" release at http://www.microsoft.com/en-us/download/details.aspx?id=41151 and the 2014 CTP at http://www.visualstudio.com/en-us/downloads/visual-studio-14-ctp-vs.aspx

# Usage

There is nothing to build, just include `variant.hpp` and `recursive_wrapper.hpp` in your project.

# Tests

The tests depend on:

 - Boost headers (for benchmarking against `boost::variant`)
 - Boost built with `--with-timer` (used for benchmark timing)

On Unix systems set your boost includes and libs locations and run `make test`:

    export LDFLAGS='-L/opt/boost/lib'
    export CXXFLAGS='-I/opt/boost/include'
    make test

On windows do:

    vcbuild

## Benchmark

On Unix systems run the benchmark like:

    make bench

## Check object sizes

    make sizes /path/to/boost/variant.hpp

