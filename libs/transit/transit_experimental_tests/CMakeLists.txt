project(transit_experimental_tests)

set(SRC
  parse_transit_from_json_tests.cpp
  transit_serdes_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} transit)
