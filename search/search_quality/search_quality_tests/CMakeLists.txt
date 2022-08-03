project(search_quality_tests)

set(SRC
  benchmark_tests.cpp
  real_mwm_tests.cpp
  sample_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  search_quality
)
