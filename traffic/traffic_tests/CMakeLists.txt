project(traffic_tests)

set(SRC traffic_info_test.cpp)

omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_QT)

target_link_libraries(${PROJECT_NAME} traffic)
