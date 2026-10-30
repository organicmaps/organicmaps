project(kml_tests)

set(SRC
  area_tests.cpp
  color_parser_tests.cpp
  geojson_tests.cpp
  gpx_tests.cpp
  minzoom_quadtree_tests.cpp
  serdes_binary_v10_tests.cpp
  serdes_tests.cpp
  tests_data.hpp
  type_utils_tests.cpp
  visibility_tests.cpp
  xml_properties_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE kml)
