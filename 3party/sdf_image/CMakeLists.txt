project(sdf_image)

add_clang_compile_options("-Wno-shorten-64-to-32")

set(
  SRC
  sdf_image.cpp
  sdf_image.h
)

add_library(${PROJECT_NAME} ${SRC})
