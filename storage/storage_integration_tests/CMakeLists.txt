project(storage_integration_tests)

set(SRC
  download_calc_size_test.cpp
  lightweight_matching_tests.cpp
  storage_3levels_tests.cpp
  storage_downloading_tests.cpp
  storage_group_download_tests.cpp
  storage_http_tests.cpp
  storage_update_tests.cpp
  test_defines.cpp
  test_defines.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_QT REQUIRE_SERVER)

target_link_libraries(${PROJECT_NAME}
  platform_tests_support
  storage
  map
)
