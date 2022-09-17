cmake_minimum_required(VERSION 3.18)
project(omim C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
# Our code does not rely on gnu extensions.
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)

if (APPLE AND NOT ("${CMAKE_SYSTEM_NAME}" STREQUAL Android))
  # OBJC/OBJCXX are needed to skip m/mm files in Unity builds.
  # https://gitlab.kitware.com/cmake/cmake/-/issues/21963
  enable_language(OBJC)
  set(CMAKE_OBJC_EXTENSIONS OFF)
  set(CMAKE_OBJC_STANDARD 11)
  set(CMAKE_OBJC_FLAGS -fobjc-arc)
  enable_language(OBJCXX)
  set(CMAKE_OBJCXX_EXTENSIONS OFF)
  set(CMAKE_OBJCXX_STANDARD 17)
  set(CMAKE_OBJCXX_FLAGS -fobjc-arc)
endif()

message(STATUS "Using compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

option(UNITY_DISABLE "Disable unity build" OFF)
if (NOT UNITY_DISABLE AND NOT DEFINED ENV{UNITY_DISABLE})
  set(CMAKE_UNITY_BUILD ON)
  set(CMAKE_UNITY_BUILD_BATCH_SIZE 50)
  message(STATUS "Using Unity Build with batch ${CMAKE_UNITY_BUILD_BATCH_SIZE}, export UNITY_DISABLE=1 or use -DUNITY_DISABLE=ON to disable it.")
endif()

option(CCACHE_DISABLE "Disable ccache" OFF)
if (NOT CCACHE_DISABLE AND NOT DEFINED ENV{CCACHE_DISABLE})
  find_program(CCACHE_PROGRAM ccache HINTS /usr/local/bin/)
  if (CCACHE_PROGRAM)
    message(STATUS "Using ccache, export CCACHE_DISABLE=1 or use -DCCACHE_DISABLE=ON to disable it.")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif()
endif()

option(COLORS_DISABLE "Disable colored compiler output" OFF)
if (NOT DEFINED ENV{COLORS_DISABLE} AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  message(STATUS "export COLORS_DISABLE=1 or use -DCOLORS_DISABLE=ON to disable colored compiler output.")
  add_compile_options($<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color=always> $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fcolor-diagnostics>)
  add_link_options($<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color=always> $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fcolor-diagnostics>)
endif()

set(OMIM_ROOT ${CMAKE_SOURCE_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OMIM_ROOT}/cmake")

include(OmimHelpers)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(LINUX_DETECTED TRUE)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Android")
  set(ANDROID_DETECTED TRUE)
  if ((${OS} MATCHES "mac"))
    set(DARWIN TRUE)
  endif()
endif()


omim_set_platform_var(PLATFORM_IPHONE "iphone-.*")
omim_set_platform_var(PLATFORM_ANDROID "android-.*" ${ANDROID_DETECTED})
omim_set_platform_var(PLATFORM_MAC "macx-.*" ${APPLE})
omim_set_platform_var(PLATFORM_WIN "win32-.*" ${WIN32})
omim_set_platform_var(PLATFORM_LINUX "linux-.*" ${LINUX_DETECTED})

if (PLATFORM_LINUX OR PLATFORM_MAC OR PLATFORM_WIN)
  set(PLATFORM_DESKTOP TRUE)
else()
  set(PLATFORM_DESKTOP FALSE)
endif()

# End of setting the target platform

# Sanitizer
if (PLATFORM_DESKTOP)
  # https://clang.llvm.org/docs/UsersManual.html#controlling-code-generation
  set(BUILD_WITH_SANITIZER None CACHE STRING "Set to 'address' or others to enable sanitizer")

  if (NOT ${BUILD_WITH_SANITIZER} MATCHES "None")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${BUILD_WITH_SANITIZER} -fno-omit-frame-pointer")
      set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=${BUILD_WITH_SANITIZER} -fno-omit-frame-pointer")
      message(STATUS "Enable sanitizer: ${BUILD_WITH_SANITIZER}")
  endif()
endif()

# Set build type:
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Global compile options for all configurations.
add_compile_options(-ffast-math)

# Built-in CMake configurations: Debug, Release, RelWithDebInfo, MinSizeRel
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  add_definitions(-DDEBUG -fno-omit-frame-pointer)
elseif (${CMAKE_BUILD_TYPE} MATCHES "Rel")
  add_definitions(-DRELEASE)
  add_compile_options(-Ofast)
else()
  message(FATAL_ERROR "Unknown build type: " ${CMAKE_BUILD_TYPE})
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
# End of setting build type

# Options

# Call `make package` after cmake to build design tool.
option(BUILD_DESIGNER "Build application as design tool" OFF)
if (BUILD_DESIGNER)
  message(STATUS "Designer tool building is enabled")
  add_definitions(-DBUILD_DESIGNER)
endif()

option(USE_ASAN "Enable Address Sanitizer" OFF)
option(USE_TSAN "Enable Thread Sanitizer" OFF)
option(USE_LIBFUZZER "Enable LibFuzzer" OFF)
option(PYBINDINGS "Create makefiles for building python bindings" OFF)
option(SKIP_DESKTOP "Skip building of desktop application" OFF)
# TODO: Fix mapshot, it doesn't work without our old FreeType hack.
option(BUILD_MAPSHOT "Build mapshot tool" OFF)
option(USE_PCH "Use precompiled headers" OFF)
option(NJOBS "Number of parallel processes" OFF)

if (NJOBS)
  message(STATUS "Number of parallel processes: ${NJOBS}")
  set(CMAKE_JOB_POOLS custom=${NJOBS})
  set(CMAKE_JOB_POOL_COMPILE custom)
  set(CMAKE_JOB_POOL_LINK custom)
  set(CMAKE_JOB_POOL_PRECOMPILE_HEADER custom)
endif()

# GCC 8.1 is required to support <charconv> header inclusion in base/string_utils.hpp, otherwise 7.0 is sufficient
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.1)
  message(FATAL_ERROR "Minimum supported g++ version is 8.1 yours is ${CMAKE_CXX_COMPILER_VERSION}")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(PCH_EXTENSION "pch")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(PCH_EXTENSION "gch")
endif()

if (PLATFORM_LINUX)
  option(USE_PPROF "Enable Google Profiler" OFF)
endif()

if (USE_ASAN)
  message(STATUS "Address Sanitizer is enabled")
endif()

if (USE_TSAN)
  message(STATUS "Thread Sanitizer is enabled")
endif()

if (USE_ASAN AND USE_TSAN)
  message(FATAL_ERROR "Can't use two different sanitizers together")
endif()

if (USE_LIBFUZZER)
  message(STATUS "LibFuzzer is enabled")
endif()

if (USE_PPROF)
  message(STATUS "Google Profiler is enabled")
  add_definitions(-DUSE_PPROF)
endif()

if (USE_HEAPPROF)
  message(STATUS "Heap Profiler is enabled")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set environment variables
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

if (PLATFORM_LINUX OR PLATFORM_ANDROID)
  find_program(LLD_FOUND ld.lld)
  if (LLD_FOUND)
    message(STATUS "Using ld.ldd linker")
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
  set(SKIP_TESTS FALSE)
endif()

if (NOT PYTHON_VERSION)
  # FDroid build server has 3.5.3 Python installed.
  set(PYTHON_VERSION 3.5)
endif()

# End of setting environment variables

find_package(Threads REQUIRED)

# Scripts

if (NOT CMAKE_HOST_WIN32)
  execute_process(
      COMMAND "${OMIM_ROOT}/tools/unix/check_cert.sh"
      RESULT_VARIABLE CheckCertResult
  )
  if (CheckCertResult)
      message(FATAL_ERROR "Certificate check failed")
  endif()
endif()

if (NOT PLATFORM_IPHONE AND NOT PLATFORM_ANDROID)
  list(APPEND qt_components Core Network)
  if (NOT SKIP_DESKTOP OR NOT SKIP_TESTS OR PYBINDINGS)
    list(APPEND qt_components Widgets)
  endif()
  if (NOT SKIP_DESKTOP)
    list(APPEND qt_components Gui Xml Svg)
  endif()
  # PATHS are hard-coded hints where to look for qt5 in addition to other places.
  find_package(Qt5 COMPONENTS REQUIRED ${qt_components} PATHS $ENV{QT_PATH} /opt/homebrew/opt/qt@5 /usr/local/opt/qt@5 /usr/lib/x86_64-linux-gnu/qt5)
  if (Qt5_VERSION VERSION_LESS 5.5.0)
    message(FATAL_ERROR "Minimum supported Qt5 version is 5.5")
  endif()
endif()

find_library(LIBZ NAMES z)
if (LIBZ STREQUAL "LIBZ-NOTFOUND")
  message(FATAL_ERROR "Failed to find libz library.")
endif()

# To allow #include "base/file_name.hpp" in all sources.
include_directories(${CMAKE_HOME_DIRECTORY})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (USE_ASAN)
  add_compile_options(
    "-fsanitize=address"
    "-fno-omit-frame-pointer"
  )
endif()

if (USE_TSAN)
  add_compile_options(
    "-fsanitize=thread"
    "-fno-omit-frame-pointer"
  )
endif()

if (USE_LIBFUZZER)
  add_compile_options(
    "-fsanitize=fuzzer"
  )
endif()

if (USE_PCH)
  message(STATUS "Precompiled headers are ON")
  set(OMIM_PCH_TARGET_NAME "omim_pch")
  add_precompiled_headers(
    ${OMIM_ROOT}/precompiled_headers.hpp
    ${OMIM_PCH_TARGET_NAME}
  )
endif()
# Include 3party dependencies.

# Configure expat library.
# Suppress "Policy CMP0077 is not set: option() honors normal variables"
# for the expat options below.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(EXPAT_BUILD_TOOLS OFF)
set(EXPAT_BUILD_EXAMPLES OFF)
set(EXPAT_BUILD_TESTS OFF)
set(EXPAT_BUILD_DOCS OFF)
set(EXPAT_BUILD_PKGCONFIG OFF)
set(EXPAT_ENABLE_INSTALL OFF)
set(EXPAT_SHARED_LIBS OFF)
add_subdirectory(3party/expat/expat)
add_subdirectory(3party/agg)
add_subdirectory(3party/bsdiff-courgette)

add_subdirectory(3party/gflags)
# Not needed for the usual build process, but it fixes QtCreator editor,
# that doesn't see gflags/gflags.h in binary dir (gflags has tricky cmake configuration).
if (PLATFORM_DESKTOP)
  include_directories("${PROJECT_BINARY_DIR}/3party/gflags/include")
endif()

if (LINUX_DETECTED)
  find_package(ICU COMPONENTS uc i18n data REQUIRED)
  find_package(Freetype REQUIRED)
else()
  add_subdirectory(3party/freetype)
  add_subdirectory(3party/icu)
endif()
add_subdirectory(3party/jansson)
add_subdirectory(3party/liboauthcpp)
add_subdirectory(3party/minizip)
add_subdirectory(3party/opening_hours)
add_subdirectory(3party/pugixml)
add_subdirectory(3party/sdf_image)
add_subdirectory(3party/stb_image)
add_subdirectory(3party/succinct)
add_subdirectory(3party/open-location-code)
add_subdirectory(3party/vulkan_wrapper)

if (PLATFORM_DESKTOP)
  add_subdirectory(3party/libtess2)
endif()

# Only options related to warnings should be placed here.
# Other options should be set before all add_subdirectory calls.
add_compile_options(
  "-Wall"
)

find_package(Python3 COMPONENTS Interpreter)
if (Python3_Interpreter_FOUND)
  message(STATUS "Found python to use in qt/, shaders/ and 3party/: ${Python3_EXECUTABLE}")
else()
  message(FATAL_ERROR "Could not find python3 to use in qt/, shaders/ and 3party/.")
endif()

add_subdirectory(base)
add_subdirectory(coding)
add_subdirectory(descriptions)
add_subdirectory(drape)
add_subdirectory(drape_frontend)
add_subdirectory(editor)
add_subdirectory(ge0)
add_subdirectory(generator/mwm_diff)
add_subdirectory(geometry)
add_subdirectory(indexer)
add_subdirectory(kml)
add_subdirectory(map)
add_subdirectory(platform)
add_subdirectory(routing)
add_subdirectory(routing_common)
add_subdirectory(search)
add_subdirectory(shaders)
add_subdirectory(storage)
add_subdirectory(tracking)
add_subdirectory(traffic)
add_subdirectory(transit)

if (PLATFORM_DESKTOP)
  if (BUILD_MAPSHOT)
    add_subdirectory(mapshot)
    add_subdirectory(software_renderer)
  endif()
  omim_add_tool_subdirectory(feature_list)
  add_subdirectory(generator)
  add_subdirectory(openlr)
  add_subdirectory(poly_borders)
  omim_add_tool_subdirectory(topography_generator)
  add_subdirectory(track_analyzing)
  omim_add_tool_subdirectory(track_generator)
  if (NOT SKIP_DESKTOP)
    add_subdirectory(qt)
    omim_add_tool_subdirectory(skin_generator)
  endif()
endif()

omim_add_test_subdirectory(qt_tstfrm)

if (PLATFORM_ANDROID)
  add_subdirectory(android/jni)
endif()
