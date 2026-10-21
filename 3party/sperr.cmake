# Source-only mirror of NCAR/SPERR b8012586908aaabba993038d183b3937c8ea9c7c.
# The local source patches are listed in sperr/README.organicmaps.
# Its standalone CMake uses top-level paths,
# enables installation/LTO and can add ISA flags to consumers.
function(omim_add_sperr)
  set(SPERR_VERSION_MAJOR 0)
  set(SPERR_VERSION_MINOR 8)
  set(SPERR_VERSION_PATCH 5)
  set(GIT_SHA1 b8012586908aaabba993038d183b3937c8ea9c7c)
  set(GIT_BRANCH pinned)
  set(USE_OMP OFF)
  configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/SperrConfig.h.in" sperr/SperrConfig.h @ONLY)
  set(sperr_helper_source "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/sperr_helper.cpp")
  if(ANDROID)
    # aligned_alloc is unavailable before Android API 28. Stage this allocation
    # adaptation in the build tree, keeping the pinned upstream source intact.
    file(READ "${sperr_helper_source}" SPERR_HELPER_SOURCE)
    string(REPLACE "return std::aligned_alloc(alignment, size);"
      "void* result = nullptr;\n  return posix_memalign(&result, alignment, size) == 0 ? result : nullptr;"
      SPERR_HELPER_SOURCE "${SPERR_HELPER_SOURCE}")
    set(sperr_helper_source "${CMAKE_CURRENT_BINARY_DIR}/sperr/sperr_helper.cpp")
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr_android_helper.cpp.in" "${sperr_helper_source}" @ONLY)
  endif()
  add_library(omim_sperr STATIC
    "${sperr_helper_source}"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/Bitstream.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/Bitmask.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/Conditioner.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/CDF97.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/SPECK_INT.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/SPECK2D_INT.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/SPECK2D_INT_ENC.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/SPECK2D_INT_DEC.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/SPECK1D_INT.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/SPECK1D_INT_ENC.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/SPECK1D_INT_DEC.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/SPECK_FLT.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/SPECK2D_FLT.cpp"
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/src/Outlier_Coder.cpp"
  )
  add_library(sperr::sperr ALIAS omim_sperr)
  target_include_directories(omim_sperr SYSTEM PUBLIC
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sperr/include"
    "${CMAKE_CURRENT_BINARY_DIR}/sperr"
  )
  set_target_properties(omim_sperr PROPERTIES UNITY_BUILD OFF POSITION_INDEPENDENT_CODE ON)
  # Upstream Bitstream.cpp uses std::fill without including <algorithm>.
  target_precompile_headers(omim_sperr PRIVATE <algorithm>)
  target_compile_options(omim_sperr PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-fast-math;-ffp-contract=off>
    $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>
  )
endfunction()
omim_add_sperr()
