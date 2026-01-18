project(timezone_tests)

set(SRC
  conversion_tests.cpp
  serdes_tests.cpp
  timezone_in_metadata_tests.cpp
  timezone_info_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC} GTEST)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  timezone
  indexer
)
