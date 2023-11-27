project(search_tests_support)

set(
  SRC
  helpers.cpp
  helpers.hpp
  test_results_matching.cpp
  test_results_matching.hpp
  test_search_engine.cpp
  test_search_engine.hpp
  test_search_request.cpp
  test_search_request.hpp
  test_with_custom_mwms.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  generator_tests_support
  editor_tests_support
  pugixml  # osm_editor.hpp -> editor_config.hpp
)
