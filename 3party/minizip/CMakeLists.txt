project(minizip)

add_definitions(-DUSE_FILE32API -DNOCRYPT)

add_clang_compile_options("-Wno-unused-value")
add_gcc_compile_options("-Wno-unused-value")

set(
  SRC
  minizip.cpp
  minizip.hpp
  src/ioapi.c
  src/ioapi.h
  src/unzip.c
  src/unzip.h
  src/zip.c
  src/zip.h
)

add_library(${PROJECT_NAME} ${SRC})
