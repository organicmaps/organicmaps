project(geometry_tests)

add_definitions(-DOMIM_UNIT_TEST_DISABLE_PLATFORM_INIT)

set(
  SRC
  algorithm_test.cpp
  angle_test.cpp
  anyrect_test.cpp
  cellid_test.cpp
  clipping_test.cpp
  common_test.cpp
  convex_hull_tests.cpp
  covering_test.cpp
  distance_on_sphere_test.cpp
  distance_test.cpp
  equality.hpp
  intersect_test.cpp
  large_polygon.hpp
  latlon_test.cpp
  nearby_points_sweeper_test.cpp
  mercator_test.cpp
  packer_test.cpp
  point_test.cpp
  pointu_to_uint64_test.cpp
  polygon_test.cpp
  rect_test.cpp
  region2d_binary_op_test.cpp
  region_test.cpp
  robust_test.cpp
  screen_test.cpp
  segments_intersect_test.cpp
  simplification_test.cpp
  spline_test.cpp
  test_regions.hpp
  transformations_test.cpp
  tree_test.cpp
  vector_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME} geometry base)
