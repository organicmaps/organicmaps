cmake_minimum_required(VERSION 2.8.8)

if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
  message(FATAL_ERROR "In-source builds are not allowed.
Please create a directory and run cmake from there, passing the path to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'. Please delete them.")
endif()

project(OSRM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CheckCXXCompilerFlag)
include(FindPackageHandleStandardArgs)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)
git_describe(GIT_DESCRIPTION)

set(bitness 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(bitness 64)
  message(STATUS "Building on a 64 bit system")
else()
  message(WARNING "Building on a 32 bit system is unsupported")
endif()

if (WIN32 AND MSVC_VERSION LESS 1800)
  message(FATAL_ERROR "Building with Microsoft compiler needs Visual Studio 2013 or later (Express version works too)")
endif()

OPTION(WITH_TOOLS "Build OSRM tools" OFF)
OPTION(BUILD_TOOLS "Build OSRM tools" OFF)

include_directories(${CMAKE_SOURCE_DIR}/Include/)
include_directories(${CMAKE_SOURCE_DIR}/../../jansson/src/)

add_custom_target(FingerPrintConfigure ALL
  ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FingerPrint-Config.cmake
  COMMENT "Configuring FingerPrint.cpp"
  VERBATIM)

set(BOOST_COMPONENTS date_time filesystem iostreams program_options regex system thread)

configure_file(
  ${CMAKE_SOURCE_DIR}/Util/GitDescription.cpp.in
  ${CMAKE_SOURCE_DIR}/Util/GitDescription.cpp
)
file(GLOB ExtractorGlob Extractor/*.cpp)
file(GLOB ExtractorGlobH Extractor/*.h)
file(GLOB ImporterGlob DataStructures/Import*.cpp)
add_library(IMPORT OBJECT ${ImporterGlob})
add_library(LOGGER OBJECT Util/simple_logger.cpp)

set(ExtractorSources extractor.cpp ${ExtractorGlob} ${ExtractorGlobH})
add_executable(osrm-extract ${ExtractorSources} $<TARGET_OBJECTS:COORDINATE> $<TARGET_OBJECTS:FINGERPRINT> $<TARGET_OBJECTS:GITDESCRIPTION> $<TARGET_OBJECTS:IMPORT> $<TARGET_OBJECTS:LOGGER>)

file(GLOB PrepareGlobH Contractor/*.h DataStructures/*.h)
file(GLOB PrepareGlob Contractor/*.cpp DataStructures/HilbertValue.cpp DataStructures/RestrictionMap.cpp Util/compute_angle.cpp)
set(PrepareSources prepare.cpp ${PrepareGlob} ${PrepareGlobH})
add_executable(osrm-prepare ${PrepareSources} $<TARGET_OBJECTS:FINGERPRINT> $<TARGET_OBJECTS:GITDESCRIPTION> $<TARGET_OBJECTS:COORDINATE> $<TARGET_OBJECTS:IMPORT> $<TARGET_OBJECTS:LOGGER>)

file(GLOB ServerGlob Server/*.cpp)
file(GLOB RoutingAlgs RoutingAlgorithms/*.h)
file(GLOB DescriptorGlob Descriptors/*.cpp)
file(GLOB DatastructureGlob DataStructures/SearchEngineData.cpp DataStructures/RouteParameters.cpp)
list(REMOVE_ITEM DatastructureGlob DataStructures/Coordinate.cpp)
file(GLOB CoordinateGlob DataStructures/Coordinate.cpp)
file(GLOB AlgorithmGlob Algorithms/*.cpp)
file(GLOB HttpGlob Server/Http/*.cpp)
file(GLOB LibOSRMGlob Library/*.cpp)

file(GLOB MapsMeSources mapsme/*.cpp)
file(GLOB MapsMeHeaders mapsme/*.h)
file(GLOB MapsMeGenerator ../../../storage/country.cpp ../../../storage/country_decl.cpp ../../../storage/country_info.cpp)

set(
  OSRMSources
  ${LibOSRMGlob}
  ${DescriptorGlob}
  ${DatastructureGlob}
  ${CoordinateGlob}
  ${AlgorithmGlob}
  ${HttpGlob}
  ${MapsMeGenerator}
)
add_library(COORDINATE OBJECT ${CoordinateGlob})
add_library(FINGERPRINT OBJECT Util/FingerPrint.cpp)
add_library(GITDESCRIPTION OBJECT Util/GitDescription.cpp)
add_library(OSRM ${OSRMSources} $<TARGET_OBJECTS:GITDESCRIPTION> $<TARGET_OBJECTS:FINGERPRINT> $<TARGET_OBJECTS:COORDINATE> $<TARGET_OBJECTS:LOGGER>)
add_dependencies(FINGERPRINT FingerPrintConfigure)

add_executable(osrm-routed routed.cpp ${ServerGlob} ${RoutingAlgs})
add_executable(osrm-datastore datastore.cpp $<TARGET_OBJECTS:COORDINATE> $<TARGET_OBJECTS:FINGERPRINT> $<TARGET_OBJECTS:GITDESCRIPTION> $<TARGET_OBJECTS:LOGGER>)
add_executable(osrm-mapsme ${MapsMeSources} ${MapsMeHeaders} "${CMAKE_SOURCE_DIR}/../../succinct/rs_bit_vector.cpp" $<TARGET_OBJECTS:FINGERPRINT> $<TARGET_OBJECTS:GITDESCRIPTION> $<TARGET_OBJECTS:COORDINATE>)


# Check the release mode
if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Configuring OSRM in debug mode")
  if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message(STATUS "adding profiling flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -fno-inline")
    set(CMAKE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -fno-inline")
  endif()
endif()
if(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Configuring OSRM in release mode")
  # Check if LTO is available
  set(LTO_FLAGS "")
  CHECK_CXX_COMPILER_FLAG("-flto" HAS_LTO_FLAG)
  if (HAS_LTO_FLAG)
    set(LTO_FLAGS "${LTO_FLAGS} -flto")

    # Since gcc 4.9 the LTO format is non-standart ('slim'), so we need to use the build-in tools
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND
        NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.9.0" AND NOT MINGW)
      message(STATUS "Using gcc specific binutils for LTO.")
      set(CMAKE_AR     "/usr/bin/gcc-ar")
      set(CMAKE_RANLIB "/usr/bin/gcc-ranlib")
    endif()
  endif (HAS_LTO_FLAG)
endif()

# Configuring compilers
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wunreachable-code -pedantic -fPIC")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -fPIC")
  if (WIN32) # using mingw
    add_definitions(-D_USE_MATH_DEFINES) # define M_PI, M_1_PI etc.
    add_definitions(-DWIN32)
    SET(OPTIONAL_SOCKET_LIBS ws2_32 wsock32)
    SET(OPTIONAL_OMP_LIB gomp)
  endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-intel -wd10237 -Wall -ipo -fPIC")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} date_time chrono zlib)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-DNOMINMAX) # avoid min and max macros that can break compilation
  add_definitions(-D_USE_MATH_DEFINES) # define M_PI
  add_definitions(-D_WIN32_WINNT=0x0501)
endif()

# Activate C++11
if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  ADD_DEFINITIONS(-std=c++11)
endif()

# Configuring other platform dependencies
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "x86_64")
  message(STATUS "Set Architecture to x64 on OS X")
  exec_program(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
  string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
  if(OSXLIBSTD)
    message(STATUS "linking against ${OSXLIBSTD}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=${OSXLIBSTD}")
  elseif(DARWIN_VERSION GREATER 12)
    message(STATUS "linking against libc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(osrm-prepare rt)
  target_link_libraries(osrm-datastore rt)
  target_link_libraries(OSRM rt)
endif()

#Check Boost
set(BOOST_MIN_VERSION "1.49.0")
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Fatal error: Boost (version >= 1.49.0) required.\n")
endif()
include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(OSRM ${Boost_LIBRARIES})
target_link_libraries(osrm-extract ${Boost_LIBRARIES})
target_link_libraries(osrm-prepare ${Boost_LIBRARIES})
target_link_libraries(osrm-routed ${Boost_LIBRARIES} ${OPTIONAL_SOCKET_LIBS} OSRM
                      debug "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libcoding.a"
                            "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libbase.a"
                            "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libgeometry.a"
                            "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libindexer.a"
                            "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libjansson.a"
                      general "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libcoding.a"
                              "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libgeometry.a"
                              "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libindexer.a"
                              "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libjansson.a"
                              "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libbase.a")
target_link_libraries(osrm-datastore ${Boost_LIBRARIES})

target_link_libraries(osrm-mapsme ${Boost_LIBRARIES} OSRM
                      debug "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libcoding.a"
                            "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libbase.a"
                            "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/librouting.a"
                            "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libindexer.a"
                            "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libgeometry.a"
                      general "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libcoding.a"
                              "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libbase.a"
                              "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/librouting.a"
                              "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libindexer.a"
                              "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libgeometry.a")

find_package(Threads REQUIRED)
target_link_libraries(osrm-extract ${CMAKE_THREAD_LIBS_INIT} ${OPTIONAL_OMP_LIB})
target_link_libraries(osrm-datastore ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(OSRM ${CMAKE_THREAD_LIBS_INIT})

find_package(TBB REQUIRED)
if(WIN32 AND CMAKE_BUILD_TYPE MATCHES Debug)
  set(TBB_LIBRARIES ${TBB_DEBUG_LIBRARIES})
endif()
target_link_libraries(osrm-datastore ${TBB_LIBRARIES})
target_link_libraries(osrm-extract ${TBB_LIBRARIES})
target_link_libraries(osrm-prepare ${TBB_LIBRARIES})
target_link_libraries(osrm-routed ${TBB_LIBRARIES})
include_directories(${TBB_INCLUDE_DIR})

find_package( Luabind REQUIRED )
include(check_luabind)

include_directories(${LUABIND_INCLUDE_DIR})
target_link_libraries(osrm-extract ${LUABIND_LIBRARY})
target_link_libraries(osrm-prepare ${LUABIND_LIBRARY})

if( LUAJIT_FOUND )
  target_link_libraries(osrm-extract ${LUAJIT_LIBRARIES})
  target_link_libraries(osrm-prepare ${LUAJIT_LIBRARIES})
else()
  target_link_libraries(osrm-extract ${LUA_LIBRARY})
  target_link_libraries(osrm-prepare ${LUA_LIBRARY})
endif()
include_directories(${LUA_INCLUDE_DIR})

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
target_link_libraries(osrm-extract ${LIBXML2_LIBRARIES})

find_package( STXXL REQUIRED )
include_directories(${STXXL_INCLUDE_DIR})
target_link_libraries(OSRM ${STXXL_LIBRARY})
target_link_libraries(osrm-extract ${STXXL_LIBRARY})
target_link_libraries(osrm-prepare ${STXXL_LIBRARY})

find_package( OSMPBF REQUIRED )
include_directories(${OSMPBF_INCLUDE_DIR})
target_link_libraries(osrm-extract ${OSMPBF_LIBRARY})
target_link_libraries(osrm-prepare ${OSMPBF_LIBRARY})

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
target_link_libraries(osrm-extract ${PROTOBUF_LIBRARY})
target_link_libraries(osrm-prepare ${PROTOBUF_LIBRARY})

find_package(BZip2 REQUIRED)
include_directories(${BZIP_INCLUDE_DIRS})
target_link_libraries(osrm-extract ${BZIP2_LIBRARIES})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(osrm-extract ${ZLIB_LIBRARY})
target_link_libraries(osrm-routed ${ZLIB_LIBRARY})

if(WITH_TOOLS OR BUILD_TOOLS)
  message(STATUS "Activating OSRM internal tools")
  find_package(GDAL)
  if(GDAL_FOUND)
    add_executable(osrm-components Tools/components.cpp $<TARGET_OBJECTS:FINGERPRINT> $<TARGET_OBJECTS:IMPORT> $<TARGET_OBJECTS:COORDINATE> $<TARGET_OBJECTS:LOGGER>)
    target_link_libraries(osrm-components ${TBB_LIBRARIES})
    include_directories(${GDAL_INCLUDE_DIR})
    target_link_libraries(
      osrm-components
      ${GDAL_LIBRARIES} ${Boost_LIBRARIES})
  else()
    message(FATAL_ERROR "libgdal and/or development headers not found")
  endif()
  add_executable(osrm-cli Tools/simpleclient.cpp $<TARGET_OBJECTS:LOGGER>)
  target_link_libraries(osrm-cli ${Boost_LIBRARIES} ${OPTIONAL_SOCKET_LIBS} OSRM)
  target_link_libraries(osrm-cli ${TBB_LIBRARIES})
  add_executable(osrm-io-benchmark Tools/io-benchmark.cpp $<TARGET_OBJECTS:GITDESCRIPTION> $<TARGET_OBJECTS:LOGGER>)
  target_link_libraries(osrm-io-benchmark ${Boost_LIBRARIES})
  add_executable(osrm-unlock-all Tools/unlock_all_mutexes.cpp $<TARGET_OBJECTS:GITDESCRIPTION> $<TARGET_OBJECTS:LOGGER>)
  target_link_libraries(osrm-unlock-all ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  if(UNIX AND NOT APPLE)
    target_link_libraries(osrm-unlock-all rt)
  endif()
endif()

file(GLOB InstallGlob Include/osrm/*.h Library/OSRM.h)

# Add RPATH info to executables so that when they are run after being installed
# (i.e., from /usr/local/bin/) the linker can find library dependencies. For
# more info see http://www.cmake.org/Wiki/CMake_RPATH_handling
set_property(TARGET osrm-extract PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-prepare PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-datastore PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
set_property(TARGET osrm-routed PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

install(FILES ${InstallGlob} DESTINATION include/osrm)
install(TARGETS osrm-extract DESTINATION bin)
install(TARGETS osrm-prepare DESTINATION bin)
install(TARGETS osrm-datastore DESTINATION bin)
install(TARGETS osrm-routed DESTINATION bin)
install(TARGETS OSRM DESTINATION lib)
list(GET Boost_LIBRARIES 1 BOOST_LIBRARY_FIRST)
get_filename_component(BOOST_LIBRARY_LISTING "${BOOST_LIBRARY_FIRST}" PATH)
set(BOOST_LIBRARY_LISTING "-L${BOOST_LIBRARY_LISTING}")
foreach (lib ${Boost_LIBRARIES})
  get_filename_component(BOOST_LIBRARY_NAME "${lib}" NAME_WE)
  string(REPLACE "lib" "" BOOST_LIBRARY_NAME ${BOOST_LIBRARY_NAME})
  set(BOOST_LIBRARY_LISTING "${BOOST_LIBRARY_LISTING} -l${BOOST_LIBRARY_NAME}")
endforeach ()

configure_file(${CMAKE_SOURCE_DIR}/cmake/pkgconfig.in libosrm.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/libosrm.pc DESTINATION lib/pkgconfig)

if(BUILD_DEBIAN_PACKAGE)
  include(CPackDebianConfig)
  include(CPack)
endif()
