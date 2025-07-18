project(storage_tests)

set(SRC
  countries_tests.cpp
  country_info_getter_tests.cpp
  country_name_getter_tests.cpp
  downloader_tests.cpp
  fake_map_files_downloader.cpp
  fake_map_files_downloader.hpp
  helpers.cpp
  helpers.hpp
  simple_tree_test.cpp
  storage_tests.cpp
  task_runner.cpp
  task_runner.hpp
  test_map_files_downloader.cpp
  test_map_files_downloader.hpp
)

omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_QT REQUIRE_SERVER)

target_link_libraries(${PROJECT_NAME}
  platform_tests_support
  generator_tests_support
  storage
  cppjansson
)
