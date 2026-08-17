project(opening_hours_tests)

set(SRC
  oh_eval_tests.cpp
  oh_parser_tests.cpp
  opening_hours_tests.cpp
  osmoh_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE opening_hours platform_tests_support)

# Real-world corpus for the parse-coverage regression test.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/opening-count.lst" "${CMAKE_BINARY_DIR}/"
  COMMENT "Copying opening-count.lst for the opening_hours coverage test")
