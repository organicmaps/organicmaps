cmake_minimum_required(VERSION 3.2)

project(omim C CXX)

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

if (PLATFORM_LINUX OR PLATFORM_MAC OR PLATFORM_WIN)
  set(PLATFORM_DESKTOP TRUE)
else()
  set(PLATFORM_DESKTOP FALSE)
endif()
# End of setting the target platform

# Options
option(USE_ASAN "Enable Address Sanitizer" OFF)
option(PYBINDINGS "Create makefiles for building python bindings" OFF)
if (PLATFORM_LINUX)
  option(USE_PPROF "Enable Google Profiler" OFF)
endif()

if (USE_ASAN)
  message("Address Sanitizer is enabled")
endif()

if (USE_PPROF)
  message("Google Profiler is enabled")
  add_definitions(-DUSE_PPROF)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set environment variables
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

if ($ENV{QT_PATH})
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} $ENV{QT_PATH})
else()
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/opt/qt5")
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  add_definitions(-DDEBUG)
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  add_definitions(-DRELEASE)
endif()

if (NOT SKIP_TESTS)
  set(SKIP_TESTS FALSE)
endif()

if (NOT PYTHON_VERSION)
  set(PYTHON_VERSION 2.7)
endif()

# End of setting environment variables

# Find installed packages

find_package(Threads)

set(Boost_USE_MULTITHREADED ON)

if (PLATFORM_MAC)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

find_package(Boost 1.54)

if (PYBINDINGS)
  if (PYTHON_VERSION VERSION_GREATER 3.0)
    set(_Boost_PYTHON3_HEADERS "boost/python.hpp")
    find_package(Boost 1.54 REQUIRED COMPONENTS python3)
  else()
    find_package(Boost 1.54 REQUIRED COMPONENTS python)
  endif()
  find_package(PythonLibs ${PYTHON_VERSION} REQUIRED)
  include_directories(${PYTHON_INCLUDE_DIRS})
endif()

if (NOT PLATFORM_IPHONE AND NOT PLATFORM_ANDROID)
  find_package(Qt5Core)
  if (NOT Qt5Core_FOUND)
    message(FATAL_ERROR "Qt5 cmake files were not found, please set QT_PATH environment variable")
  endif()
  find_package(Qt5Network REQUIRED)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5OpenGL REQUIRED)
  find_package(Qt5Widgets REQUIRED)
endif()

if (PLATFORM_LINUX)
  find_package(OpenGL)
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
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Network_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

# Functions for using in subdirectories
function(omim_add_executable executable)
  add_executable(${executable} ${ARGN})
  if (USE_ASAN)
    target_link_libraries(${executable} "-fsanitize=address" "-fno-omit-frame-pointer")
  endif()
  if (USE_PPROF)
    target_link_libraries(${executable} "-lprofiler")
  endif()
endfunction()

function(omim_add_test executable)
  if (NOT SKIP_TESTS)
    omim_add_executable(${executable} ${OMIM_ROOT}/testing/testingmain.cpp ${ARGN})
  endif()
endfunction()

function(omim_add_test_subdirectory subdir)
  if (NOT SKIP_TESTS)
    add_subdirectory(${subdir})
  else()
    message("SKIP_TESTS: Skipping subdirectory ${subdir}")
  endif()
endfunction()

function(omim_add_pybindings_subdirectory subdir)
  if (PYBINDINGS)
    add_subdirectory(${subdir})
  else()
    message("Skipping pybindings subdirectory ${subdir}")
  endif()
endfunction()

function(omim_link_platform_deps target)
  if ("${ARGN}" MATCHES "platform")
    if (PLATFORM_MAC)
      target_link_libraries(
        ${target}
        "-framework CFNetwork"
        "-framework Foundation"
        "-framework IOKit"
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

function(link_opengl target)
    if (PLATFORM_MAC)
      omim_link_libraries(
        ${target}
        "-framework OpenGL"
      )
    endif()

    if (PLATFORM_LINUX)
      omim_link_libraries(
        ${target}
        ${OPENGL_gl_LIBRARY}
      )
    endif()
endfunction()

function(link_qt5_core target)
  omim_link_libraries(
    ${target}
    ${Qt5Core_LIBRARIES}
  )

  if (PLATFORM_MAC)
    omim_link_libraries(
      ${target}
      "-framework IOKit"
    )
  endif()
endfunction()

function(link_qt5_network target)
  omim_link_libraries(
    ${target}
    ${Qt5Network_LIBRARIES}
  )
endfunction()

# End of functions for subdirectories

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include subdirectories
add_subdirectory(3party/jansson)
add_subdirectory(3party/minizip)
add_subdirectory(3party/freetype)
add_subdirectory(3party/icu)
add_subdirectory(3party/expat)
add_subdirectory(map)

if (PLATFORM_DESKTOP)
  add_subdirectory(3party/libtess2)
endif()

add_compile_options(
  "-Wall"
  "-std=c++11"
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    "-Wshorten-64-to-32"
  )
endif()

if (USE_ASAN)
  add_compile_options(
    "-fsanitize=address"
    "-fno-omit-frame-pointer"
  )
endif()

add_subdirectory(3party/stb_image)
add_subdirectory(3party/sdf_image)
add_subdirectory(3party/protobuf)
add_subdirectory(3party/liboauthcpp)
add_subdirectory(3party/pugixml)
add_subdirectory(3party/succinct)
add_subdirectory(3party/osrm)
add_subdirectory(3party/gflags)
add_subdirectory(base)
add_subdirectory(coding)
add_subdirectory(geometry)
add_subdirectory(platform)
add_subdirectory(3party/opening_hours)
add_subdirectory(stats)
add_subdirectory(drape)
add_subdirectory(drape_frontend)
add_subdirectory(storage)
add_subdirectory(editor)
add_subdirectory(indexer)
add_subdirectory(routing)
add_subdirectory(routing_common)
add_subdirectory(search)
add_subdirectory(tracking)
add_subdirectory(traffic)
add_subdirectory(partners_api)
add_subdirectory(local_ads)

if (PLATFORM_DESKTOP)
  add_subdirectory(openlr)
  add_subdirectory(generator)
endif()

omim_add_test_subdirectory(qt_tstfrm)
omim_add_test_subdirectory(3party/gmock)

if (NOT PLATFORM_IPHONE AND NOT PLATFORM_ANDROID AND NOT SKIP_DESKTOP)
    add_subdirectory(qt)
endif()
