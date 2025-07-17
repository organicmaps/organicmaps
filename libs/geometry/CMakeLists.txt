project(geometry)

set(SRC
  algorithm.cpp
  algorithm.hpp
  angles.cpp
  angles.hpp
  any_rect2d.hpp
  area_on_earth.cpp
  area_on_earth.hpp
  avg_vector.hpp
  bounding_box.cpp
  bounding_box.hpp
  calipers_box.cpp
  calipers_box.hpp
  cellid.hpp
  circle_on_earth.cpp
  circle_on_earth.hpp
  clipping.cpp
  clipping.hpp
  convex_hull.cpp
  convex_hull.hpp
  covering.hpp
  covering_utils.hpp
  diamond_box.cpp
  diamond_box.hpp
  distance_on_sphere.cpp
  distance_on_sphere.hpp
  latlon.cpp
  latlon.hpp
  line2d.cpp
  line2d.hpp
  mercator.cpp
  mercator.hpp
  meter.hpp
  nearby_points_sweeper.cpp
  nearby_points_sweeper.hpp
  oblate_spheroid.cpp
  oblate_spheroid.hpp
  packer.cpp
  packer.hpp
  parametrized_segment.hpp
  point2d.hpp
  point3d.hpp
  point_with_altitude.cpp
  point_with_altitude.hpp
  polygon.hpp
  intersection_score.hpp
  polyline2d.hpp
  rect2d.hpp
  rect_intersect.hpp
  region2d.hpp
  robust_orientation.cpp
  robust_orientation.hpp
  screenbase.cpp
  screenbase.hpp
  segment2d.cpp
  segment2d.hpp
  simplification.hpp
  smoothing.cpp
  smoothing.hpp
  spline.cpp
  spline.hpp
  transformations.hpp
  tree4d.hpp
  triangle2d.cpp
  triangle2d.hpp
)

if (NOT PLATFORM_IPHONE AND NOT PLATFORM_ANDROID)
  append(SRC
    region2d/binary_operators.cpp
    region2d/binary_operators.hpp
    region2d/boost_concept.hpp
  )
endif()

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} base)

omim_add_test_subdirectory(geometry_tests)
