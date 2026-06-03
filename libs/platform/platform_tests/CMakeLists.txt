project(platform_tests)

set(SRC
  cjk_resolver_tests.cpp
  country_file_tests.cpp
  distance_tests.cpp
  duration_tests.cpp
  downloader_tests/downloader_test.cpp
  downloader_utils_tests.cpp
  http_client_test.cpp
  get_text_by_id_tests.cpp
  glaze_test.cpp
  language_test.cpp
  local_country_file_tests.cpp
  location_test.cpp
  measurement_tests.cpp
  platform_test.cpp
  meta_config_tests.cpp
  products_tests.cpp
  utm_mgrs_utils_tests.cpp
)

# Qt HTTP backend tests are only valid where http_client_qt.cpp is compiled
# (macOS uses http_client_apple.mm instead).
if (PLATFORM_DESKTOP AND NOT PLATFORM_MAC)
  list(APPEND SRC http_client_qt_retry_test.cpp)
  list(APPEND SRC http_client_qt_http2_test.cpp)
endif()

omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_QT REQUIRE_SERVER)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    platform_tests_support
    platform
    glaze::glaze
)
