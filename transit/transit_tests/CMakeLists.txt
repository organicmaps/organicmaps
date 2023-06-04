project(transit_tests)

set(SRC
  transit_graph_test.cpp
  transit_json_parsing_test.cpp
  transit_schedule_tests.cpp
  transit_test.cpp
  transit_tools.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} transit)
