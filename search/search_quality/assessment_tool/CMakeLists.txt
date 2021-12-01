project(assessment_tool)

set(SRC
  assessment_tool.cpp
  context.cpp
  context.hpp
  edits.cpp
  edits.hpp
  feature_info_dialog.cpp
  feature_info_dialog.hpp
  helpers.cpp
  helpers.hpp
  main_model.cpp
  main_model.hpp
  main_view.cpp
  main_view.hpp
  model.hpp
  result_view.cpp
  result_view.hpp
  results_view.cpp
  results_view.hpp
  sample_view.cpp
  sample_view.hpp
  samples_view.cpp
  samples_view.hpp
  search_request_runner.cpp
  search_request_runner.hpp
  view.hpp
)

#omim_add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SRC})
omim_add_executable(${PROJECT_NAME} ${SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  search_quality
  qt_common
  map
  gflags::gflags
)

# if (PLATFORM_MAC)
#   set_target_properties(
#     ${PROJECT_NAME}
#     PROPERTIES
#     MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/Info.plist
#   )
# endif()
