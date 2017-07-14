project(sdf_image)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    "-Wno-shorten-64-to-32"
  )
endif()

set(
  SRC
  sdf_image.cpp
  sdf_image.h
)

add_library(${PROJECT_NAME} ${SRC})
