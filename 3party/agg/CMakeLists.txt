project(agg)

add_clang_compile_options("-Wno-deprecated-declarations")
add_gcc_compile_options("-Wno-deprecated-declarations")

set(CMAKE_PREFIX_PATH ./)

set(
  SRC
  agg_curves.cpp
  agg_vcgen_stroke.cpp
)

add_library(${PROJECT_NAME} ${SRC})
