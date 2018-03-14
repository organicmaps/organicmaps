project(kml)

set(
  SRC
  header_binary.hpp
  serdes.cpp
  serdes.hpp
  serdes_binary.cpp
  serdes_binary.hpp
  type_utils.hpp
  types.hpp
  visitors.hpp
)

add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(kml_tests)
