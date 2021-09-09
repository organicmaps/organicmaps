project(world_roads_builder_tool)

set(
 SRC
 world_roads_builder_tool.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} gflags::gflags)

omim_link_libraries(
  ${PROJECT_NAME}
  world_roads_builder
)

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
  )
endif()

if (PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    dl
  )
endif()

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})
