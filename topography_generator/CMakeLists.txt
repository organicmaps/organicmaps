project(topography_generator_tool)

set(SRC
  filters_impl.cpp
  filters_impl.hpp
  generator.cpp
  generator.hpp
  isolines_utils.hpp
  isolines_profile.hpp
  main.cpp
  marching_squares/contours_builder.cpp
  marching_squares/contours_builder.hpp
  marching_squares/marching_squares.hpp
  marching_squares/square.hpp
  tile_filter.hpp
  utils/contours.hpp
  utils/contours_serdes.hpp
  utils/values_provider.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  generator
  storage
  indexer
  platform
  coding
  gflags::gflags
)
