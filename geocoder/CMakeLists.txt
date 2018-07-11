project(geocoder)

set(
  SRC
  geocoder.cpp
  geocoder.hpp
  hierarchy.cpp
  hierarchy.hpp
  result.cpp
  result.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(geocoder_tests)
