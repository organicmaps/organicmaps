project(network_tests)

set(SRC
  downloader/chunks_downloader.cpp
  downloader/simple_downloader.cpp
  downloader/utils.cpp
  chunks_download_strategy.cpp
  servers_list.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_SERVER GTEST)

target_link_libraries(${PROJECT_NAME}
  network
  network_tests_support
  platform
  GTest::gtest_main
  gmock
  Qt6::Core
  Qt6::Widgets
)
