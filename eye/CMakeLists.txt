project(eye)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(
  SRC
  eye.cpp
  eye.hpp
  eye_info.hpp
  eye_serdes.cpp
  eye_serdes.hpp
  eye_storage.cpp
  eye_storage.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(eye_tests)
omim_add_test_subdirectory(eye_tests_support)
