project(sdf_image)

set(SRC
  sdf_image.cpp
  sdf_image.h
)

add_library(${PROJECT_NAME} ${SRC})

target_compile_options(${PROJECT_NAME}
  PRIVATE $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wno-shorten-64-to-32>
)
