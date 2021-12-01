project(traffic)

set(SRC
  speed_groups.cpp
  speed_groups.hpp
  traffic_cache.cpp
  traffic_cache.hpp
  traffic_info.cpp
  traffic_info.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  routing_common
  indexer
)

omim_add_test_subdirectory(traffic_tests)
