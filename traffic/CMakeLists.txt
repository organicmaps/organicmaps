project(traffic)

set(
  SRC
  speed_groups.cpp
  speed_groups.hpp
  traffic_info.cpp
  traffic_info.hpp
)

add_library(${PROJECT_NAME} ${SRC})

add_subdirectory(pytraffic)
omim_add_test_subdirectory(traffic_tests)
