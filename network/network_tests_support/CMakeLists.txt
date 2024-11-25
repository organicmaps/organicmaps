project(network_tests_support)

set(SRC
  request_fixture.hpp
  test_socket.cpp
  test_socket.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  network
  GTest::gtest_main
  gmock
  Qt6::Core
  Qt6::Widgets
)
