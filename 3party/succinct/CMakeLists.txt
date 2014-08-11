cmake_minimum_required(VERSION 2.6)
project(SUCCINCT)

configure_file(
  ${SUCCINCT_SOURCE_DIR}/succinct_config.hpp.in
  ${SUCCINCT_SOURCE_DIR}/succinct_config.hpp)

option(SUCCINCT_USE_LIBCXX
  "Use libc++ with Clang instead of libstdc++ (must be same as that used to compile Boost)"
  OFF)
option(SUCCINCT_USE_INTRINSICS
  "Use a set of intrinsics available on all x86-64 architectures"
  ON)
option(SUCCINCT_USE_POPCNT
  "Use popcount intrinsic. Available on x86-64 since SSE4.2."
  OFF)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   if (SUCCINCT_USE_LIBCXX)
     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   endif ()
endif ()

if (SUCCINCT_USE_POPCNT)
  if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
  endif ()
  # XXX(ot): what to do for MSVC?
endif ()


# XXX(ot): enable this on all compilers
if (UNIX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-braces")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion")
endif ()


find_package(Boost 1.42.0 COMPONENTS
  unit_test_framework iostreams system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})

include_directories(${PROJECT_SOURCE_DIR})

set(SUCCINCT_SOURCES
  rs_bit_vector.cpp
  bp_vector.cpp
  )

add_library(succinct STATIC ${SUCCINCT_SOURCES})

add_subdirectory(perftest)

# make and run tests only if library is compiled stand-alone
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  enable_testing()
  file(GLOB SUCCINCT_TEST_SOURCES test_*.cpp)
  foreach(TEST_SRC ${SUCCINCT_TEST_SOURCES})
    get_filename_component (TEST_SRC_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_SRC_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_SRC_NAME}
      succinct
      ${Boost_LIBRARIES}
      )
    add_test(${TEST_SRC_NAME} ${TEST_SRC_NAME})
  endforeach(TEST_SRC)
endif ()
