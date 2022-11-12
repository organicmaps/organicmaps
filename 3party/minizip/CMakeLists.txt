project(minizip)

set(SRC
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

target_compile_definitions(${PROJECT_NAME}
  PUBLIC
    USE_FILE32API
  PRIVATE
    NOCRYPT
    NOUNCRYPT
)

target_compile_options(${PROJECT_NAME} PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang,GNU>:-Wno-unused-value>)

target_link_libraries(${PROJECT_NAME} zlib)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}\\..\\zlib" src)
