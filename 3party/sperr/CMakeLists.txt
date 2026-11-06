# Source-only mirror of NCAR/SPERR b8012586908aaabba993038d183b3937c8ea9c7c, the local source
# patches are listed in README.organicmaps. Builds only the 2D codec sources, without OpenMP,
# installation or ISA flags, and with strict floating point, so the decoded integer heights
# match on every platform. It replaces upstream CMake, which assumes it is the top-level project.
# The iOS build is xcode/sperr/sperr.xcodeproj.
project(omim_sperr LANGUAGES CXX)

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
target_include_directories(omim_sperr SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(omim_sperr PROPERTIES UNITY_BUILD OFF POSITION_INDEPENDENT_CODE ON)
# Overrides the global -ffast-math: the generator corrects the patch perimeters against its own decode.
target_compile_options(omim_sperr PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-fast-math;-ffp-contract=off>
  $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>
)
