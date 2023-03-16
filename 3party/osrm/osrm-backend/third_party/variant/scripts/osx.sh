#!/usr/bin/env bash

set -e -u
set -o pipefail

# install boost headers
brew unlink boost
brew install boost

# run tests
make test
make bench
make clean

# compare object sizes against boost::variant
make sizes `brew --prefix`/include/boost/variant.hpp
make clean

# test building with gyp
make gyp