cmake_minimum_required(VERSION 3.2)

project(omim C CXX)

add_compile_options(
    "-Wall"
    "-pedantic"
    "-std=c++11"
)

find_package(Threads)

get_filename_component(PROJECT_SOURCE_DIR . ABSOLUTE)

include_directories(${CMAKE_HOME_DIRECTORY} ${PROJECT_SOURCE_DIR}/3party/jansson/src)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

if ("${NO_TESTS}" STREQUAL "")
    set(NO_TESTS FALSE)
endif()

function(omim_add_library library)
    add_library(${library} ${ARGN})
endfunction()

function(omim_add_executable executable)
    add_executable(${executable} ${ARGN})
endfunction()

function(omim_add_test executable)
    if (NOT ${NO_TESTS})
        omim_add_executable(${executable} ${PROJECT_SOURCE_DIR}/testing/testingmain.cpp ${ARGN})
    endif()
endfunction()

function(omim_link_libraries target)
    if (TARGET ${target})
        target_link_libraries(${target} ${ARGN} ${CMAKE_THREAD_LIBS_INIT})
    else()
        message("~> Skipping linking the libraries to the target ${target} as it does not exist")
    endif()
endfunction()


add_subdirectory(base)
add_subdirectory(geometry)
add_subdirectory(storage)
