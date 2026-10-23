# Header-only subset of szcompressor/SZ3 v3.4.0, see README.organicmaps.
add_library(sz3 INTERFACE)
target_include_directories(sz3 SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(sz3::sz3 ALIAS sz3)
