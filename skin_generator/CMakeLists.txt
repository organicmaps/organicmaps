project(skin_generator_tool)

add_compile_options(
  "-std=c++11"
  "-Wno-c++11-narrowing"
)

include_directories(
  ${OMIM_ROOT}/3party/gflags/src
  ${OMIM_ROOT}/3party/freetype/include
  ${Boost_INCLUDE_DIRS}
)

set(
  SRC
  generator.cpp
  generator.hpp
  main.cpp
)

if (BUILD_DESIGNER)
  set(SRC MACOSX_BUNDLE ${SRC})
endif()

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  geometry
  coding
  base
  freetype
  gflags
  ${Boost_LIBRARIES}
  ${Qt5Gui_LIBRARIES}
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Xml_LIBRARIES}
  ${Qt5Svg_LIBRARIES}
  ${LIBZ}
)
