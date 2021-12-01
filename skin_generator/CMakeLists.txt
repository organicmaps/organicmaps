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
  Qt5::Xml
  Qt5::Svg
  Qt5::Widgets
)
