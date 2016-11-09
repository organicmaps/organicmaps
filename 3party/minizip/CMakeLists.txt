project(minizip)

add_definitions(-DUSE_FILE32ZPI -DNOCRYPT)

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
