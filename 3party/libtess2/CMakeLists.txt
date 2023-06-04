project(tess2)

include_directories(Include)

set(
  SRC
  Include/tesselator.h
  Source/bucketalloc.c
  Source/bucketalloc.h
  Source/dict.c
  Source/dict.h
  Source/geom.c
  Source/geom.h
  Source/mesh.c
  Source/mesh.h
  Source/priorityq.c
  Source/priorityq.h
  Source/sweep.c
  Source/sweep.h
  Source/tess.c
  Source/tess.h
)

add_library(${PROJECT_NAME} ${SRC})
