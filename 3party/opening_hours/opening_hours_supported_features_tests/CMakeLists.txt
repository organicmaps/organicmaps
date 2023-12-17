project(opening_hours_supported_features_tests)

set(SRC opening_hours_supported_features_tests.cpp)

omim_add_test(${PROJECT_NAME} ${SRC} BOOST_TEST)

target_link_libraries(${PROJECT_NAME} opening_hours)
