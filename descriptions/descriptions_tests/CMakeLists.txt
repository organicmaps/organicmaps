project(descriptions_tests)

set(SRC
  descriptions_tests.cpp
)

omim_add_test_no_platform_init(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} descriptions)
