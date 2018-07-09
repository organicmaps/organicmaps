project(transit)

include_directories(
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  transit_display_info.hpp
  transit_graph_data.cpp
  transit_graph_data.hpp
  transit_serdes.hpp
  transit_speed_limits.hpp
  transit_types.cpp
  transit_types.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(transit_tests)
