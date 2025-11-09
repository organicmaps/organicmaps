project(address_parser_tests)
omim_add_test(${PROJECT_NAME} parser_tests.cpp)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    address_parser
    generator_tests_support
)
