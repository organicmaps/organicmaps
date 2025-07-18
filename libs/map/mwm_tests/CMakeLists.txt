project(mwm_tests)

set(SRC
  multithread_mwm_test.cpp
  mwm_foreach_test.cpp
  mwm_index_test.cpp
  world_map_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} map)
