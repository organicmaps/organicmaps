project(platform_tests)

set(SRC
  apk_test.cpp
  country_file_tests.cpp
  downloader_tests/downloader_test.cpp
  downloader_utils_tests.cpp
  get_text_by_id_tests.cpp
  jansson_test.cpp
  language_test.cpp
  local_country_file_tests.cpp
  location_test.cpp
  measurement_tests.cpp
  platform_test.cpp
  utm_mgrs_utils_tests.cpp
)

omim_add_test_with_qt_event_loop(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  platform_tests_support
  platform
  cppjansson
)
