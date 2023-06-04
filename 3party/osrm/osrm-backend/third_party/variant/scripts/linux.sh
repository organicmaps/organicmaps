#!/usr/bin/env bash

set -e -u
set -o pipefail

# ppa for latest boost
sudo add-apt-repository -y ppa:boost-latest/ppa
# ppa for g++ 4.7 and 4.8
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -y

# install boost headers and g++ upgrades
sudo apt-get -y -qq install boost1.55 gcc-4.8 g++-4.8 gcc-4.7 g++-4.7

if [[ "$CXX" == "clang++" ]]; then
    echo 'running tests against clang++'
    make test
    make bench
    make clean
else
    # run tests against g++ 4.7
    export CXX="g++-4.7"; export CC="gcc-4.7"
    echo 'running tests against g++ 4.7'
    make test
    make bench
    make clean

    # run tests against g++ 4.8
    export CXX="g++-4.8"; export CC="gcc-4.8"
    echo 'running tests against g++ 4.8'
    make test
    make bench
    make clean

fi

# compare object sizes against boost::variant
echo 'comparing object sizes to boost::variant'
make sizes /usr/include/boost/variant.hpp
make clean

# test building with gyp
echo 'testing build with gyp'
make gyp

# run coverage when using clang++
if [[ $CXX == "clang++" ]]; then
    make clean
    make coverage
    git status
    ./out/cov-test
    cp unit*gc* test/
    sudo pip install cpp-coveralls
    coveralls -i variant.hpp -i recursive_wrapper.hpp --gcov-options '\-lp'
fi

# set strictness back to normal
# to avoid tripping up travis
set +e +u
