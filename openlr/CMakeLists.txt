project(openlr)

set(
  SRC
  openlr_model.cpp
  openlr_model.hpp
  openlr_sample.cpp
  openlr_sample.hpp
  openlr_simple_decoder.cpp
  openlr_simple_decoder.hpp
  openlr_simple_parser.cpp
  openlr_simple_parser.hpp
  road_info_getter.cpp
  road_info_getter.hpp
  road_type_checkers.cpp
  road_type_checkers.hpp
  router.cpp
  router.hpp
  way_point.hpp
)

add_library(${PROJECT_NAME} ${SRC})
add_subdirectory(openlr_stat)
omim_add_test_subdirectory(openlr_tests)
