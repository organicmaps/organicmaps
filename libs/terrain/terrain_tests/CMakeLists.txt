project(terrain_tests)

set(SRC
  isolines_tracer_tests.cpp
  sz3_decoder_tests.cpp
  terrain_utils_tests.cpp
  tile_mesh_tests.cpp
  twm_set_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    platform_tests_support
    terrain
    sz3::sz3
)
# Same as terrain/sz3_decoder.cpp: the linker keeps one copy of the SZ3 templates both instantiate,
# and SZ3/def.hpp rejects fast math.
set_source_files_properties(sz3_decoder_tests.cpp PROPERTIES COMPILE_OPTIONS
  "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-fast-math;-ffp-contract=off>;$<$<CXX_COMPILER_ID:MSVC>:/fp:strict>")
set_property(SOURCE sz3_decoder_tests.cpp APPEND PROPERTY COMPILE_DEFINITIONS
  "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:SZ3_ASSUME_STRICT_FP>")
