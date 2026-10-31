# Source-only mirror of NCAR/SPERR b8012586908aaabba993038d183b3937c8ea9c7c, the local source
# patches are listed in README.organicmaps. Builds only the 2D codec sources, without OpenMP,
# installation or ISA flags, and with strict floating point, so the decoded integer heights
# match on every platform. It replaces upstream CMake, which assumes it is the top-level project.
# Added by 3party/CMakeLists.txt, and built standalone by tools/autobuild/build_sperr_xcode.sh.
cmake_minimum_required(VERSION 3.22.1)
project(omim_sperr LANGUAGES CXX)

if (PROJECT_IS_TOP_LEVEL)
  set(CMAKE_CXX_STANDARD 23)
endif()

set(SPERR_VERSION_MAJOR 0)
set(SPERR_VERSION_MINOR 8)
set(SPERR_VERSION_PATCH 5)
set(GIT_SHA1 b8012586908aaabba993038d183b3937c8ea9c7c)
set(GIT_BRANCH pinned)
set(USE_OMP OFF)
configure_file(SperrConfig.h.in sperr/SperrConfig.h @ONLY)

add_library(omim_sperr STATIC
  src/sperr_helper.cpp
  src/Bitstream.cpp
  src/Bitmask.cpp
  src/Conditioner.cpp
  src/CDF97.cpp
  src/SPECK_INT.cpp
  src/SPECK2D_INT.cpp
  src/SPECK2D_INT_ENC.cpp
  src/SPECK2D_INT_DEC.cpp
  src/SPECK1D_INT.cpp
  src/SPECK1D_INT_ENC.cpp
  src/SPECK1D_INT_DEC.cpp
  src/SPECK_FLT.cpp
  src/SPECK2D_FLT.cpp
  src/Outlier_Coder.cpp
)
add_library(sperr::sperr ALIAS omim_sperr)
target_include_directories(omim_sperr SYSTEM PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_BINARY_DIR}/sperr"
)
set_target_properties(omim_sperr PROPERTIES UNITY_BUILD OFF POSITION_INDEPENDENT_CODE ON)
target_compile_options(omim_sperr PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-fast-math;-ffp-contract=off>
  $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>
)
