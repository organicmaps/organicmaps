project(geometry_tests)

set(SRC
  algorithm_test.cpp
  angle_test.cpp
  anyrect_test.cpp
  area_on_earth_tests.cpp
  bounding_box_tests.cpp
  calipers_box_tests.cpp
  cellid_test.cpp
  circle_on_earth_tests.cpp
  clipping_test.cpp
  common_test.cpp
  convex_hull_tests.cpp
  covering_test.cpp
  diamond_box_tests.cpp
  distance_on_sphere_test.cpp
  equality.hpp
  intersect_test.cpp
  intersection_score_tests.cpp
  large_polygon.hpp
  latlon_test.cpp
  line2d_tests.cpp
  mercator_test.cpp
  nearby_points_sweeper_test.cpp
  oblate_spheroid_tests.cpp
  packer_test.cpp
  parametrized_segment_tests.cpp
  point3d_tests.cpp
  point_test.cpp
  polygon_test.cpp
  polyline_tests.cpp
  rect_test.cpp
  region2d_binary_op_test.cpp
  region_tests.cpp
  robust_test.cpp
  screen_test.cpp
  segment2d_tests.cpp
  simplification_test.cpp
  spline_test.cpp
  test_regions.hpp
  transformations_test.cpp
  tree_test.cpp
  vector_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC} NO_PLATFORM_INIT)

target_link_libraries(${PROJECT_NAME} geometry)
