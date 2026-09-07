project(transit)

set(SRC
  transit_display_info.hpp
  transit_graph_data.cpp
  transit_graph_data.hpp
  transit_serdes.hpp
  transit_types.cpp
  transit_types.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    coding
    glaze::glaze
    geometry
    indexer
)

omim_add_test_subdirectory(transit_tests)
