project(descriptions)

set(SRC
  header.hpp
  loader.cpp
  loader.hpp
  serdes.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  indexer  # MwmHandle
  coding
)

omim_add_test_subdirectory(descriptions_tests)
