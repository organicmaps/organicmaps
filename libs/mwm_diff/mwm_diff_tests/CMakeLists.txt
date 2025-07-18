project(mwm_diff_tests)

set(SRC
  diff_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} mwm_diff)
