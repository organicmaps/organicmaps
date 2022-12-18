project(kml_tests)

set(SRC
  serdes_tests.cpp
  tests_data.hpp
  minzoom_quadtree_tests.cpp
  gpx_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  kml
)
