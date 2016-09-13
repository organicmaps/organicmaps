cmake_minimum_required(VERSION 3.2)

project(minizip C CXX)

add_compile_options(
    "-Wall"
    "-pedantic"
)

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

add_library(minizip ${SRC})
set_target_properties(minizip PROPERTIES ENABLE_EXPORTS ON)
