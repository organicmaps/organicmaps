add_subdirectory(openlr)
add_subdirectory(poly_borders)
add_subdirectory(track_analyzing)

omim_add_tool_subdirectory(feature_list)
omim_add_tool_subdirectory(topography_generator)
omim_add_tool_subdirectory(track_generator)
if (NOT SKIP_QT_GUI)
  omim_add_tool_subdirectory(skin_generator)
endif()

