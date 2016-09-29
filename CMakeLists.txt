cmake_minimum_required(VERSION 3.2)

project(omim C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add third-party subdirectories


# Find installed packages

find_package(Threads)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

if (APPLE)
    find_library(APL_SYSCON SystemConfiguration)
endif()

find_library(LIBZ NAMES z)
if (LIBZ STREQUAL "LIBZ-NOTFOUND")
   message(FATAL_ERROR "Failed to find libz library.")
endif()


get_filename_component(PROJECT_SOURCE_DIR . ABSOLUTE)

include_directories(
    ${CMAKE_HOME_DIRECTORY}
    ${Qt5Core_LIBRARIES}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Network_LIBRARIES}
    ${Qt5Network_INCLUDE_DIRS}
    "${CMAKE_HOME_DIRECTORY}/3party/boost"
)

# Set environment variables

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

# End of setting environment variables

# Set target platform:

function(omim_set_platform_var PLATFORM_VAR pattern)
    set(${PLATFORM_VAR} FALSE PARENT_SCOPE)

    if ("${PLATFORM}" STREQUAL "")
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

omim_set_platform_var(PL_IPHONE "iphone-.*")
omim_set_platform_var(PL_ANDROID "android-.*")
omim_set_platform_var(PL_MAC "macx-.*" ${APPLE})
omim_set_platform_var(PL_WIN "win32-.*" ${WIN32})

if (${UNIX} AND (NOT ${PL_MAC}))
  set(LINUX_DETECTED TRUE)
else()
  set(LINUX_DETECTED FALSE)
endif()

omim_set_platform_var(PL_LINUX "linux-.*" ${LINUX_DETECTED})

# End of setting the target platform

# Functions for using in subdirectories

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

# End of functions for subdirectories

# Include subdirectories
add_subdirectory(3party/minizip)
add_subdirectory(3party/succinct)
add_subdirectory(3party/tomcrypt)
add_subdirectory(3party/jansson)

add_compile_options(
    "-Wall"
    "-std=c++11"
)

add_subdirectory(base)
add_subdirectory(coding)
#add_subdirectory(editor)
add_subdirectory(geometry)
add_subdirectory(platform)
#add_subdirectory(storage)
