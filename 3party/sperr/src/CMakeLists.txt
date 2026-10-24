add_library( SPERR
             sperr_helper.cpp
             Bitstream.cpp
             Bitmask.cpp
             Conditioner.cpp
             CDF97.cpp
             SPECK_INT.cpp
             SPECK3D_INT.cpp
             SPECK3D_INT_ENC.cpp
             SPECK3D_INT_DEC.cpp
             SPECK2D_INT.cpp
             SPECK2D_INT_ENC.cpp
             SPECK2D_INT_DEC.cpp
             SPECK1D_INT.cpp
             SPECK1D_INT_ENC.cpp
             SPECK1D_INT_DEC.cpp
             SPECK_FLT.cpp
             SPECK3D_FLT.cpp
             SPECK2D_FLT.cpp
             SPECK1D_FLT.cpp
             SPERR3D_OMP_C.cpp
             SPERR3D_OMP_D.cpp
             SPERR3D_Stream_Tools.cpp
             Outlier_Coder.cpp
             SPERR_C_API.cpp )
             
target_include_directories( SPERR PUBLIC ${CMAKE_SOURCE_DIR}/include )

if(USE_OMP)
  target_compile_options(   SPERR PUBLIC ${OpenMP_CXX_FLAGS} )
  target_link_libraries(    SPERR PUBLIC OpenMP::OpenMP_CXX )
endif()

if(ENABLE_AVX2)
  message(STATUS "AVX2 compilation enabled.")
  target_compile_options(SPERR PRIVATE
                        "$<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>"
                        "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-mavx2;-mfma>")

  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|MSVC")
    message(WARNING "Compiler '${CMAKE_CXX_COMPILER_ID}' might not explicitly support AVX2 flags. Please add them manually if needed.")
  endif()
else()
  message(STATUS "AVX2 compilation disabled.")
endif()


#
# LTO
#
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported LANGUAGES CXX)
if( ipo_supported )
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET SPERR PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported")
endif()

set_target_properties( SPERR PROPERTIES VERSION ${SPERR_VERSION} )

#
# The list of headers is formatted a little cumbersome, but don't change it!
#
set( public_h_list 
"include/sperr_helper.h;\
include/Bitstream.h;\
include/Bitmask.h;\
include/Conditioner.h;\
include/CDF97.h;\
include/SPECK_INT.h;\
include/SPECK3D_INT.h;\
include/SPECK3D_INT_ENC.h;\
include/SPECK3D_INT_DEC.h;\
include/SPECK2D_INT.h;\
include/SPECK2D_INT_ENC.h;\
include/SPECK2D_INT_DEC.h;\
include/SPECK1D_INT.h;\
include/SPECK1D_INT_ENC.h;\
include/SPECK1D_INT_DEC.h;\
include/SPECK_FLT.h;\
include/SPECK3D_FLT.h;\
include/SPECK2D_FLT.h;\
include/SPECK1D_FLT.h;\
include/SPERR3D_OMP_C.h;\
include/SPERR3D_Stream_Tools.h;\
include/SPERR3D_OMP_D.h;\
include/Outlier_Coder.h;\
include/SPERR_C_API.h;")
set_target_properties( SPERR PROPERTIES PUBLIC_HEADER "${public_h_list}" )

