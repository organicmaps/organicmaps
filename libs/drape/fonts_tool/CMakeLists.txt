project(fonts_tool)

set(SRC
  fonts_tool.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} drape)
