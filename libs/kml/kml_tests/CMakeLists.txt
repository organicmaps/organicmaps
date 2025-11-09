project(kml_tests)

set(SRC
  color_parser_tests.cpp
  gpx_tests.cpp
  minzoom_quadtree_tests.cpp
  serdes_tests.cpp
  tests_data.hpp
  geojson_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE kml)
