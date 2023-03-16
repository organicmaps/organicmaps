project(traffic_tests)

set(SRC traffic_info_test.cpp)

omim_add_test_with_qt_event_loop(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} traffic)
