project(search_quality)

include_directories(${OMIM_ROOT}/3party/jansson/src)

set(SRC
  helpers.cpp
  helpers.hpp
  matcher.cpp
  matcher.hpp
  sample.cpp
  sample.hpp
)

add_library(${PROJECT_NAME} ${SRC})

if (NOT SKIP_DESKTOP)
  add_subdirectory(assessment_tool)
endif()

add_subdirectory(features_collector_tool)
add_subdirectory(search_quality_tool)
omim_add_test_subdirectory(search_quality_tests)
