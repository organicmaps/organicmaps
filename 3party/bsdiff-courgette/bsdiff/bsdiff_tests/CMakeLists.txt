project(bsdiff_tests)

set(SRC bsdiff_search_tests.cpp)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} bsdiff)
