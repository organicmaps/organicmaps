project(opening_hours_tests)

set(SRC opening_hours_tests.cpp)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE opening_hours)

# Real-world corpus for the parse-coverage regression test.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/opening-count.lst" "${CMAKE_BINARY_DIR}/"
  COMMENT "Copying opening-count.lst for the opening_hours coverage test")
