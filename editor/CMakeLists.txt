project(editor)

set(
  SRC
  changeset_wrapper.cpp
  changeset_wrapper.hpp
  config_loader.cpp
  config_loader.hpp
  editor_config.cpp
  editor_config.hpp
  editor_notes.cpp
  editor_notes.hpp
  editor_storage.cpp
  editor_storage.hpp
  feature_matcher.cpp
  feature_matcher.hpp
  opening_hours_ui.cpp
  opening_hours_ui.hpp
  osm_auth.cpp
  osm_auth.hpp
  server_api.cpp
  server_api.hpp
  ui2oh.cpp
  ui2oh.hpp
  user_stats.cpp
  user_stats.hpp
  xml_feature.cpp
  xml_feature.hpp
  yes_no_unknown.hpp
)

add_library(${PROJECT_NAME} ${SRC})

omim_add_test_subdirectory(editor_tests)
omim_add_test_subdirectory(osm_auth_tests)
