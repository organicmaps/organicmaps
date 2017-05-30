project(skin_generator_cmake)

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

omim_add_executable(skin_generator ${SRC})

omim_link_libraries(
  skin_generator
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
