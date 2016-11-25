cmake_minimum_required(VERSION 3.2)

project(omim C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set environment variables

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  add_definitions(-DDEBUG)
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  add_definitions(-DRELEASE)
endif()

if (NOT NO_TESTS)
  set(NO_TESTS FALSE)
endif()

# End of setting environment variables

# Set target platform:

function(omim_set_platform_var PLATFORM_VAR pattern)
  set(${PLATFORM_VAR} FALSE PARENT_SCOPE)

  if (NOT PLATFORM)
    if (${ARGN})
      list(GET ARGN 0 default_case)
      if (${default_case})
        set(${PLATFORM_VAR} TRUE PARENT_SCOPE)
        message("Setting ${PLATFORM_VAR} to true")
      endif()
    endif()
  else()
    message("Platform: ${PLATFORM}")
    if (${PLATFORM} MATCHES ${pattern})
      set(${PLATFORM_VAR} TRUE PARENT_SCOPE)
    endif()
  endif()
endfunction()

omim_set_platform_var(PLATFORM_IPHONE "iphone-.*")
omim_set_platform_var(PLATFORM_ANDROID "android-.*")
omim_set_platform_var(PLATFORM_MAC "macx-.*" ${APPLE})
omim_set_platform_var(PLATFORM_WIN "win32-.*" ${WIN32})

if (UNIX AND (NOT PLATFORM_MAC))
  set(LINUX_DETECTED TRUE)
else()
  set(LINUX_DETECTED FALSE)
endif()

omim_set_platform_var(PLATFORM_LINUX "linux-.*" ${LINUX_DETECTED})

# End of setting the target platform

# Find installed packages

find_package(Threads)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
if (NOT PLATFORM_IPHONE AND NOT PLATFORM_ANDROID)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5OpenGL REQUIRED)
  find_package(Qt5Widgets REQUIRED)
endif()

find_library(LIBZ NAMES z)
if (LIBZ STREQUAL "LIBZ-NOTFOUND")
  message(FATAL_ERROR "Failed to find libz library.")
endif()

if (NOT DEVELOPER_FRAMEWORKS_DIR)
  message("Doing nothing, because we know nothing about developer frameworks dir")
  #do nothing
else()
  include_directories(${DEVELOPER_FRAMEWORKS_DIR})
endif()

get_filename_component(OMIM_ROOT . ABSOLUTE)

include_directories(
  ${CMAKE_HOME_DIRECTORY}
  ${Qt5Core_LIBRARIES}
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Network_LIBRARIES}
  ${Qt5Network_INCLUDE_DIRS}
  ${CMAKE_HOME_DIRECTORY}/3party/boost
)

# Functions for using in subdirectories

function(omim_add_test executable)
  if (NOT NO_TESTS)
    add_executable(${executable} ${OMIM_ROOT}/testing/testingmain.cpp ${ARGN})
  endif()
endfunction()

function(omim_add_test_subdirectory subdir)
  if (NOT NO_TESTS)
    add_subdirectory(${subdir})
  endif()
endfunction()

function(omim_link_platform_deps target)
  if ("${ARGN}" MATCHES "platform")
    if (PLATFORM_MAC)
      target_link_libraries(
        ${target}
        "-framework CFNetwork"
        "-framework Foundation"
        "-framework SystemConfiguration"
      )
    endif()
  endif()
endfunction()

function(omim_link_libraries target)
  if (TARGET ${target})
    target_link_libraries(${target} ${ARGN} ${CMAKE_THREAD_LIBS_INIT})
    omim_link_platform_deps(${target} ${ARGN})
  else()
    message("~> Skipping linking the libraries to the target ${target} as it does not exist")
  endif()
endfunction()

function(append VAR)
  set(${VAR} ${${VAR}} ${ARGN} PARENT_SCOPE)
endfunction()

macro(check_pybindings)
  if (NOT PYBINDINGS)
    message("Skipping " + ${PROJECT_NAME} + " because the PYBINDINGS variale is not set or is set to FALSE")
    return()
  endif()
endmacro()

# End of functions for subdirectories

# Include subdirectories
add_subdirectory(3party/jansson)
add_subdirectory(3party/minizip)
add_subdirectory(3party/freetype)
add_subdirectory(3party/fribidi)
add_subdirectory(3party/expat)
add_subdirectory(map)
add_subdirectory(drape)

add_compile_options(
  "-Wall"
  "-std=c++11"
)

add_subdirectory(3party/protobuf)
add_subdirectory(3party/liboauthcpp)
add_subdirectory(3party/pugixml)
add_subdirectory(3party/succinct)
add_subdirectory(3party/osrm)
add_subdirectory(3party/gmock)
add_subdirectory(qt_tstfrm)
add_subdirectory(base)
add_subdirectory(coding)
add_subdirectory(geometry)
add_subdirectory(platform)
add_subdirectory(3party/opening_hours)
add_subdirectory(stats)
add_subdirectory(drape_frontend)
add_subdirectory(storage)
add_subdirectory(editor)
add_subdirectory(indexer)
add_subdirectory(routing)
add_subdirectory(search)
add_subdirectory(tracking)
add_subdirectory(traffic)
add_subdirectory(partners_api)
