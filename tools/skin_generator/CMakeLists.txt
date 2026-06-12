project(skin_generator_tool)

omim_add_library(skin_generator
  generator.cpp
  generator.hpp
)

target_link_libraries(skin_generator
  PUBLIC
    geometry
    Qt6::Gui
    Qt6::Svg
  PRIVATE
    Qt6::Xml
)

omim_add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    skin_generator
    gflags::gflags
    Qt6::Widgets
)
