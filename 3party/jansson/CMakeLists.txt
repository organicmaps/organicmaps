project(jansson)

include_directories(src ../../)

set(
  SRC
  jansson_handle.cpp
  jansson_handle.hpp
  myjansson.cpp
  myjansson.hpp
  src/dump.c
  src/error.c
  src/hashtable.c
  src/hashtable.h
  src/jansson.h
  src/jansson_config.h
  src/jansson_private.h
  src/load.c
  src/memory.c
  src/strbuffer.c
  src/strbuffer.h
  src/strconv.c
  src/utf.c
  src/utf.h
  src/value.c
)

add_library(${PROJECT_NAME} ${SRC})
