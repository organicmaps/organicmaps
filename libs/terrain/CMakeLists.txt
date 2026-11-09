project(terrain)

set(SRC
  isolines_tracer.cpp
  isolines_tracer.hpp
  raster_patch.cpp
  raster_patch.hpp
  sperr_decoder.cpp
  sperr_decoder.hpp
  sz3_decoder.cpp
  sz3_decoder.hpp
  sz3_profile.hpp
  terrain_data.cpp
  terrain_data.hpp
  terrain_normals.cpp
  terrain_normals.hpp
  terrain_reader.cpp
  terrain_reader.hpp
  terrain_serdes.hpp
  terrain_utils.cpp
  terrain_utils.hpp
  tile_mesh.cpp
  tile_mesh.hpp
  twm_grid.cpp
  twm_grid.hpp
  twm_set.cpp
  twm_set.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
# The raster decoders override the global -ffast-math like 3party/sperr: the generator corrects the patch
# perimeters against its own decode, so every platform must reconstruct the same integer heights.
# Under strict floating point SZ3 drops its FMA barriers, see 3party/sz3/README.organicmaps.
set_source_files_properties(sperr_decoder.cpp sz3_decoder.cpp PROPERTIES COMPILE_OPTIONS
  "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-fast-math;-ffp-contract=off>;$<$<CXX_COMPILER_ID:MSVC>:/fp:strict>")
set_property(SOURCE sz3_decoder.cpp APPEND PROPERTY COMPILE_DEFINITIONS
  "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:SZ3_ASSUME_STRICT_FP>")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    indexer
  PRIVATE
    sperr::sperr
    sz3::sz3
)

omim_add_test_subdirectory(terrain_tests)
