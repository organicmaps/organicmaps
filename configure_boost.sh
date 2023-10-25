#!/usr/bin/env bash
set -e -u

BASE_PATH=$(cd "$(dirname "$0")"; pwd)

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    JOBS_COUNT=$(( $(nproc) * 20 ))
elif [[ "$OSTYPE" == "darwin"* ]]; then
    JOBS_COUNT=$(( $(sysctl -n hw.logicalcpu) * 20 ))
else
    JOBS_COUNT=10
fi

git submodule update --init --checkout -- 3party/boost
cd "$BASE_PATH/3party/boost/"

BOOST_SUBMODULES=(
  libs/algorithm
  libs/array
  libs/assert
  libs/bind
  libs/circular_buffer
  libs/concept_check
  libs/config
  libs/container
  libs/container_hash
  libs/core
  libs/date_time
  libs/describe
  libs/detail
  libs/endian
  libs/exception
  libs/function
  libs/function_types
  libs/fusion
  libs/geometry
  libs/gil
  libs/integer
  libs/intrusive
  libs/io
  libs/iostreams
  libs/iterator
  libs/lambda
  libs/lexical_cast
  libs/math
  libs/move
  libs/mp11
  libs/mpl
  libs/multiprecision
  libs/numeric
  libs/optional
  libs/phoenix
  libs/polygon
  libs/predef
  libs/preprocessor
  libs/proto
  libs/qvm
  libs/random
  libs/range
  libs/rational
  libs/serialization
  libs/smart_ptr
  libs/spirit
  libs/stacktrace
  libs/static_assert
  libs/test
  libs/throw_exception
  libs/tokenizer
  libs/tti
  libs/tuple
  libs/type_index
  libs/type_traits
  libs/typeof
  libs/utility
  libs/uuid
  libs/variant
  tools
)

git submodule update --depth 1 --init --recursive --jobs=$JOBS_COUNT ${BOOST_SUBMODULES[@]}

if [[ "$OSTYPE" == msys ]]; then
  echo "For Windows please run:"
  echo "cd 3party\\boost"
  echo "bootstrap.bat"
  echo "b2 headers"
  echo "cd ..\\.."
else
  ./bootstrap.sh
  ./b2 headers
fi
cd "$BASE_PATH"
