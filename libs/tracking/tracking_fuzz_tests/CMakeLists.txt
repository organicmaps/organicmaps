project(tracking_fuzz_tests)

set(SRC
  tracking_fuzz_tests.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} tracking)
