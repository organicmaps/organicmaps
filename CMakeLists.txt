cmake_minimum_required(VERSION 3.22.1)

project(omim C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)
# Our code does not rely on gnu extensions.
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Fixes warning ld: warning: ignoring duplicate libraries on Mac and Windows.
if (POLICY CMP0156)
  cmake_policy(SET CMP0156 NEW)
endif()

set(OMIM_ROOT ${CMAKE_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OMIM_ROOT}/cmake")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include(OmimPlatform)
include(OmimOptions)
include(OmimConfig)
include(OmimHelpers)
include(OmimTesting)

if (APPLE AND NOT ("${CMAKE_SYSTEM_NAME}" STREQUAL Android))
  # OBJC/OBJCXX are needed to skip m/mm files in Unity builds.
  # https://gitlab.kitware.com/cmake/cmake/-/issues/21963
  enable_language(OBJC)
  set(CMAKE_OBJC_EXTENSIONS OFF)
  set(CMAKE_OBJC_STANDARD 23)
  set(CMAKE_OBJC_FLAGS -fobjc-arc)
  set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
  enable_language(OBJCXX)
  set(CMAKE_OBJCXX_EXTENSIONS OFF)
  set(CMAKE_OBJCXX_STANDARD 23)
  set(CMAKE_OBJCXX_FLAGS -fobjc-arc)
  set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
endif()

message(STATUS "Using compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

if (CMAKE_UNITY_BUILD)
  message(STATUS "Using Unity Build with batch ${CMAKE_UNITY_BUILD_BATCH_SIZE}, use -DCMAKE_UNITY_BUILD=OFF to disable it.")
endif()

if (USE_CCACHE)
  find_program(CCACHE_PROGRAM ccache HINTS /usr/local/bin/)
  if (CCACHE_PROGRAM)
    message(STATUS "Using ccache, use -DUSE_CCACHE=OFF to disable it.")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif()
endif()

if(${PLATFORM_MAC})
  set(XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)

  # Metal language support
  list(APPEND CMAKE_MODULE_PATH ${OMIM_ROOT}/3party/CMake-MetalShaderSupport/cmake)
  include(CheckLanguage)
  include(CMakeMetalInformation)
  include(MetalShaderSupport)
  check_language(Metal)
  if(CMAKE_Metal_COMPILER)
    enable_language(Metal)
  endif()
endif()

# Global compile options for all configurations.
if (MSVC)
  add_compile_options(/utf-8)
  add_link_options(/INCREMENTAL:NO)
else()
  add_compile_options(-ffast-math $<$<CXX_COMPILER_ID:GNU>:-Wno-psabi>)
endif()

if (PLATFORM_WIN)
  add_definitions(
    -DWIN32_LEAN_AND_MEAN
    -DNOMINMAX
  )
endif()

# Built-in CMake configurations: Debug, Release, RelWithDebInfo, MinSizeRel
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  add_definitions(-DDEBUG)
  if (NOT MSVC)
    add_compile_options(-fno-omit-frame-pointer)
  endif()
elseif (${CMAKE_BUILD_TYPE} MATCHES "Rel")
  add_definitions(-DRELEASE)
  if (NOT MSVC)
    add_compile_options(-O3)
  endif()
else()
  message(FATAL_ERROR "Unknown build type: " ${CMAKE_BUILD_TYPE})
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

if (PLATFORM_LINUX OR PLATFORM_ANDROID)
  find_program(LLD_FOUND ld.lld)
  if (LLD_FOUND)
    message(STATUS "Using ld.lld linker")
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
  else()
    find_program(GOLD_FOUND ld.gold)
    if (GOLD_FOUND)
      message(STATUS "Using ld.gold")
      set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=gold")
    endif()
  endif()
endif()

if (NOT SKIP_TESTS)
  enable_testing()
  # Enables ctest -T memcheck with valgrind
  include(CTest)
  if (COVERAGE_REPORT)
    include(OmimCoverage)
  endif ()
endif()

if (NOT PYTHON_VERSION)
  # FDroid build server has 3.5.3 Python installed.
  set(PYTHON_VERSION 3.5)
endif()

# End of setting environment variables

find_package(Threads REQUIRED)

# Scripts

if (NOT PLATFORM_IPHONE AND NOT PLATFORM_ANDROID)
  list(APPEND qt_components Core Network)
  if (NOT SKIP_QT_GUI OR NOT SKIP_TESTS OR PYBINDINGS)
    list(APPEND qt_components Widgets)
  endif()
  if (NOT SKIP_QT_GUI)
    list(APPEND qt_components Gui Xml Svg OpenGL OpenGLWidgets)
  endif()
  # PATHS are hard-coded hints where to look for qt6 in addition to other places.
  find_package(Qt6 COMPONENTS REQUIRED ${qt_components} PATHS $ENV{QT_PATH} /opt/homebrew/opt/qt@6 /usr/local/opt/qt@6 /usr/lib/x86_64-linux-gnu/qt6)

  set(MINIMUM_REQUIRED_QT_VERSION 6.4.0)
  if (Qt6Widgets_VERSION VERSION_LESS ${MINIMUM_REQUIRED_QT_VERSION})
    message(FATAL_ERROR "Unsupported Qt version: ${Qt6Widgets_VERSION}, the minimum required is ${MINIMUM_REQUIRED_QT_VERSION}")
  else()
    message(STATUS "Found Qt version: ${Qt6Widgets_VERSION}")
  endif()
endif()

# To allow #include "base/file_name.hpp" in all sources.
include_directories("${CMAKE_HOME_DIRECTORY}" "${CMAKE_HOME_DIRECTORY}/libs" "${CMAKE_HOME_DIRECTORY}/tools")

if (USE_PCH)
  message(STATUS "Precompiled headers are ON")
  set(OMIM_PCH_TARGET_NAME "omim_pch")
  add_precompiled_headers(
    ${OMIM_ROOT}/precompiled_headers.hpp
    ${OMIM_PCH_TARGET_NAME}
  )
endif()

# Should be on the root level, not in 3party, so tests can get these dependencies.
if (PLATFORM_LINUX)
  find_package(ICU COMPONENTS uc i18n data REQUIRED)
  find_package(Freetype REQUIRED)
  find_package(harfbuzz REQUIRED)
endif()

if (WITH_SYSTEM_PROVIDED_3PARTY)
  set(GFLAGS_USE_TARGET_NAMESPACE ON)
  find_package(gflags REQUIRED)

  find_package(expat CONFIG REQUIRED)
  find_package(jansson CONFIG REQUIRED)
  find_package(pugixml REQUIRED)
  find_package(utf8cpp REQUIRED)
endif()

find_package(ZLIB REQUIRED)

# Include 3party dependencies.
add_subdirectory(3party)

# Not needed for the usual build process, but it fixes QtCreator editor,
# that doesn't see gflags/gflags.h in binary dir (gflags has tricky cmake configuration).
if (PLATFORM_DESKTOP AND NOT WITH_SYSTEM_PROVIDED_3PARTY)
  include_directories("${PROJECT_BINARY_DIR}/3party/gflags/include")
endif()

# Used in qt/ and shaders/
find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_subdirectory(libs)

if (PLATFORM_DESKTOP)
  add_subdirectory(dev_sandbox)
  add_subdirectory(generator)
  add_subdirectory(tools)
  if (NOT SKIP_QT_GUI)
    add_subdirectory(qt)
  endif()
endif()

omim_add_test_subdirectory(libs/qt_tstfrm)

if (PLATFORM_ANDROID)
  add_subdirectory(android/sdk/src/main/cpp)
endif()
