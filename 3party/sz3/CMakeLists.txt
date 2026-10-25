# Header-only subset of szcompressor/SZ3 v3.4.0, see README.organicmaps.
add_library(sz3 INTERFACE)
target_include_directories(sz3 SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
# Every source including SZ3 builds with -fno-fast-math -ffp-contract=off, see README.organicmaps.
target_compile_definitions(sz3 INTERFACE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:SZ3_ASSUME_STRICT_FP>)
add_library(sz3::sz3 ALIAS sz3)
