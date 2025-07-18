project(search_integration_tests)

set(SRC
  downloader_search_test.cpp
  generate_tests.cpp
  postcode_points_tests.cpp
  pre_ranker_test.cpp
  processor_test.cpp
  ranker_test.cpp
  search_edited_features_test.cpp
  smoke_test.cpp
  tracer_tests.cpp
  utils_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  generator_tests_support
  search_tests_support
  platform_tests_support
  generator
  search
)
