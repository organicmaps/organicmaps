project(mwm_diff)

set(
  SRC
  diff.cpp
  diff.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_add_pybindings_subdirectory(pymwm_diff)
omim_add_test_subdirectory(mwm_diff_tests)
