project(search_quality)

set(SRC
  helpers.cpp
  helpers.hpp
  helpers_json.cpp
  helpers_json.hpp
  matcher.cpp
  matcher.hpp
  sample.cpp
  sample.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  search_tests_support  # TestSearchEngine in helpers.cpp
  search
  indexer
)

if (NOT SKIP_DESKTOP)
  add_subdirectory(assessment_tool)
endif()

add_subdirectory(features_collector_tool)
add_subdirectory(samples_generation_tool)
add_subdirectory(search_quality_tool)

omim_add_test_subdirectory(search_quality_tests)
