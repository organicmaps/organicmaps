project(kml_tests)

set(SRC
  color_parser_tests.cpp
  geojson_tests.cpp
  gpx_tests.cpp
  minzoom_quadtree_tests.cpp
  serdes_tests.cpp
  svg_render_tests.cpp
  tests_data.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  kml
  lunasvg
)
