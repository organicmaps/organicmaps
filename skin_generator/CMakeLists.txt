project(skin_generator_tool)

set(SRC
  generator.cpp
  generator.hpp
  main.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  geometry
  gflags::gflags
  Qt6::Xml
  Qt6::Svg
  Qt6::Widgets
)
