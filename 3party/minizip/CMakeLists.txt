project(minizip)

add_definitions(-DUSE_FILE32ZPI -DNOCRYPT)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    "-Wno-unused-value"
  )
endif()

set(
  SRC
  ioapi.c
  ioapi.h
  unzip.c
  unzip.h
  zip.c
  zip.h
)

add_library(${PROJECT_NAME} ${SRC})
