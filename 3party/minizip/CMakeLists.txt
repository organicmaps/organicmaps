project(minizip)

add_definitions(-DUSE_FILE32API -DNOCRYPT)

if (ANDROID_DETECTED)
  if (DARWIN)
    add_definitions(-D__APPLE__)
  elseif(UNIX)
    add_definitions(-DIOAPI_NO_64)
  endif()
endif()

add_clang_compile_options("-Wno-unused-value")

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
