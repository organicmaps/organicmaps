project(ugc)

set(
  SRC
  api.cpp
  api.hpp
  serdes.hpp
  storage.cpp
  storage.hpp
  types.hpp
)

add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(ugc_tests)
