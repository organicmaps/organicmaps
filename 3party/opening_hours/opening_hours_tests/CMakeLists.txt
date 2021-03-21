project(opening_hours_tests)

include_directories(${PROJECT_SOURCE_DIR} ${OMIM_ROOT}/3party/opening_hours)

set(
  SRC
  opening_hours_tests.cpp
)

# Not using omim_add_test because we don't need testingmain.cpp
omim_add_executable(${PROJECT_NAME} ${SRC})
omim_link_libraries(${PROJECT_NAME} base opening_hours)
add_dependencies(tests ${PROJECT_NAME})
