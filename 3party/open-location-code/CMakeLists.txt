project(openlocationcode)

include_directories(src ../../)

set(
  SRC
  codearea.cc
  codearea.h
  openlocationcode.cc
  openlocationcode.h
)

add_library(${PROJECT_NAME} ${SRC})
