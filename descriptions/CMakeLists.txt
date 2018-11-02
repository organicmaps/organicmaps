project(descriptions)

set(
  SRC
  header.hpp
  loader.cpp
  loader.hpp
  serdes.cpp
  serdes.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(descriptions_tests)
