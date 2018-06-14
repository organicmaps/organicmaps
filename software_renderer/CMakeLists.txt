project(software_renderer)

include_directories(
  .
  ${OMIM_ROOT}/3party/protobuf/protobuf/src
  ${OMIM_ROOT}/3party/expat/lib
  ${OMIM_ROOT}/3party/freetype/include
  ${OMIM_ROOT}/3party/glm
)

set(
  SRC
  area_info.hpp
  brush_info.hpp
  circle_info.hpp
  cpu_drawer.cpp
  cpu_drawer.hpp
  default_font.cpp
  feature_processor.cpp
  feature_processor.hpp
  feature_styler.cpp
  feature_styler.hpp
  frame_image.hpp
  geometry_processors.cpp
  geometry_processors.hpp
  glyph_cache.cpp
  glyph_cache.hpp
  glyph_cache_impl.cpp
  glyph_cache_impl.hpp
  icon_info.hpp
  path_info.hpp
  pen_info.hpp
  point.h
  proto_to_styles.cpp
  proto_to_styles.hpp
  rect.h
  software_renderer.cpp
  software_renderer.hpp
  text_engine.cpp
  text_engine.h
)

omim_add_library(${PROJECT_NAME} ${SRC})
