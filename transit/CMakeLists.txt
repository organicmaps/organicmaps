project(transit)

include_directories(
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  experimental/transit_data.cpp
  experimental/transit_data.hpp
  experimental/transit_types_experimental.cpp
  experimental/transit_types_experimental.hpp
  transit_display_info.hpp
  transit_entities.hpp
  transit_graph_data.cpp
  transit_graph_data.hpp
  transit_serdes.hpp
  transit_types.cpp
  transit_types.hpp
  transit_schedule.cpp
  transit_schedule.hpp
  transit_version.cpp
  transit_version.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(transit_experimental_tests)
omim_add_test_subdirectory(transit_tests)

if (PLATFORM_DESKTOP)
    add_subdirectory(world_feed)
endif()
