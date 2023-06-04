#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#-----------------------------------------------------------------------------
#
#  Project version
#
#-----------------------------------------------------------------------------

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;Dev"
    CACHE STRING
    "List of available configuration types"
    FORCE)

project(libosmium)

set(LIBOSMIUM_VERSION_MAJOR 2)
set(LIBOSMIUM_VERSION_MINOR 1)
set(LIBOSMIUM_VERSION_PATCH 0)

set(LIBOSMIUM_VERSION
    ${LIBOSMIUM_VERSION_MAJOR}.${LIBOSMIUM_VERSION_MINOR}.${LIBOSMIUM_VERSION_PATCH})


#-----------------------------------------------------------------------------
#
#  Build options
#
#  (Change with -DOPTION=VALUE on cmake command line.)
#
#-----------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    set(dev_build ON)
else()
    set(dev_build OFF)
endif()

option(BUILD_EXAMPLES   "compile example programs" ON)
option(BUILD_TESTING    "compile unit tests, please run them with ctest" ON)

option(BUILD_HEADERS    "compile every header file on its own" ${dev_build})
option(BUILD_BENCHMARKS "compile benchmark programs" ${dev_build})
option(BUILD_DATA_TESTS "compile data tests, please run them with ctest" ${dev_build})


#-----------------------------------------------------------------------------
#
#  Find external dependencies
#
#-----------------------------------------------------------------------------

find_package(Boost 1.38)
mark_as_advanced(CLEAR BOOST_ROOT)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
else()
    set(BOOST_ROOT "NOT FOUND: please choose" CACHE PATH "")
    message(FATAL_ERROR "PLEASE, specify the directory where the Boost library is installed in BOOST_ROOT")
endif()

set(OSMIUM_INCLUDE_DIR include)
find_package(Osmium COMPONENTS io gdal geos proj sparsehash)
include_directories(${OSMIUM_INCLUDE_DIRS})

if(MSVC)
    find_path(GETOPT_INCLUDE_DIR getopt.h)
    find_library(GETOPT_LIBRARY NAMES wingetopt)
    if(GETOPT_INCLUDE_DIR AND GETOPT_LIBRARY)
        include_directories(${GETOPT_INCLUDE_DIR})
        list(APPEND OSMIUM_LIBRARIES ${GETOPT_LIBRARY})
    else()
        set(GETOPT_MISSING 1)
    endif()
endif()

include_directories(include)


#-----------------------------------------------------------------------------
#
#  Decide which C++ version to use (Minimum/default: C++11).
#
#-----------------------------------------------------------------------------
if(NOT MSVC)
    if(NOT USE_CPP_VERSION)
        set(USE_CPP_VERSION c++11)
    endif()
    message(STATUS "Use C++ version: ${USE_CPP_VERSION}")
    # following only available from cmake 2.8.12:
    #   add_compile_options(-std=${USE_CPP_VERSION})
    # so using this instead:
    add_definitions(-std=${USE_CPP_VERSION})
endif()


#-----------------------------------------------------------------------------
#
#  Compiler and Linker flags
#
#-----------------------------------------------------------------------------
if(MSVC)
    set(USUAL_COMPILE_OPTIONS "/Ox")
else()
    set(USUAL_COMPILE_OPTIONS "-O3 -g")
endif()

if(WIN32)
    add_definitions(-DWIN32 -D_WIN32 -DMSWIN32 -DBGDWIN32
                    -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -D_WIN32_IE=0x0600)
endif()

set(CMAKE_CXX_FLAGS_DEV "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during developer builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEV ""
    CACHE STRING "Flags used by the linker during developer builds."
    FORCE)
mark_as_advanced(
    CMAKE_CXX_FLAGS_DEV
    CMAKE_EXE_LINKER_FLAGS_DEV
)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during RELWITHDEBINFO builds."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Build Type
#
#-----------------------------------------------------------------------------
# In 'Dev' mode: compile with very strict warnings and turn them into errors.
if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    if(NOT MSVC)
        add_definitions(-Werror)
    endif()
    add_definitions(${OSMIUM_WARNING_OPTIONS})
endif()

# Force RelWithDebInfo build type if none was given
if(CMAKE_BUILD_TYPE)
    set(build_type ${CMAKE_BUILD_TYPE})
else()
    set(build_type "RelWithDebInfo")
endif()

set(CMAKE_BUILD_TYPE ${build_type}
    CACHE STRING
    "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Unit and data tests
#
#-----------------------------------------------------------------------------
enable_testing()

if(BUILD_TESTING OR BUILD_DATA_TESTS)
    find_program(MEMORYCHECK_COMMAND valgrind)

    set(MEMORYCHECK_COMMAND_OPTIONS
        "--trace-children=yes --leak-check=full --show-reachable=yes --error-exitcode=1")

    set(MEMORYCHECK_SUPPRESSIONS_FILE "${PROJECT_SOURCE_DIR}/test/valgrind.supp")
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(BUILD_DATA_TESTS)
    add_subdirectory(test/data-tests)
endif()


#-----------------------------------------------------------------------------
#
#  Optional "cppcheck" target that checks C++ code
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for cppcheck")
find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    message(STATUS "Looking for cppcheck - found")
    set(CPPCHECK_OPTIONS
        --enable=warning,style,performance,portability,information,missingInclude)

    # cpp doesn't find system includes for some reason, suppress that report
    set(CPPCHECK_OPTIONS ${CPPCHECK_OPTIONS} --suppress=missingIncludeSystem)

    file(GLOB_RECURSE ALL_INCLUDES   include/osmium/*.hpp)
    file(GLOB         ALL_EXAMPLES   examples/*.cpp)
    file(GLOB         ALL_UNIT_TESTS test/t/*/test_*.cpp)
    file(GLOB         ALL_DATA_TESTS test/data-tests/*.cpp)

    if(Osmium_DEBUG)
        message(STATUS "Checking includes      : ${ALL_INCLUDES}")
        message(STATUS "Checking example code  : ${ALL_EXAMPLES}")
        message(STATUS "Checking unit test code: ${ALL_UNIT_TESTS}")
        message(STATUS "Checking data test code: ${ALL_DATA_TESTS}")
    endif()

    set(CPPCHECK_FILES
        ${ALL_INCLUDES}
        ${ALL_EXAMPLES}
        ${ALL_UNIT_TESTS}
        ${ALL_DATA_TESTS})

    add_custom_target(cppcheck
        ${CPPCHECK}
        --std=c++11 ${CPPCHECK_OPTIONS}
        -I ${CMAKE_SOURCE_DIR}/include
        ${CPPCHECK_FILES}
    )
else()
    message(STATUS "Looking for cppcheck - not found")
    message(STATUS "  Build target 'cppcheck' will not be available.")
endif(CPPCHECK)


#-----------------------------------------------------------------------------
#
#  Examples, benchmarks and documentation
#
#-----------------------------------------------------------------------------

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

add_subdirectory(doc)


#-----------------------------------------------------------------------------
#
#  Headers
#
#  This will try to compile include files on their own to detect missing
#  include directives and other dependency-related problems. Note that if this
#  work, it is not enough to be sure it will compile in production code.
#  But if it reports an error we know we are missing something.
#
#-----------------------------------------------------------------------------
if(BUILD_HEADERS)
    file(GLOB_RECURSE
         ALL_HPPS
         RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
         include/osmium/*.hpp)

    file(MAKE_DIRECTORY header_check)

    foreach(hpp ${ALL_HPPS})
        string(REPLACE ".hpp" "" tmp ${hpp})
        string(REPLACE "/" "__" libname ${tmp})

        # Create a dummy .cpp file that includes the header file we want to
        # check.
        set(DUMMYCPP ${CMAKE_BINARY_DIR}/header_check/${libname}.cpp)
        file(WRITE ${DUMMYCPP} "#include <${hpp}>\n")

        # There is no way in CMake to just compile but not link a C++ file,
        # so we pretend to build a library here.
        add_library(${libname} STATIC ${DUMMYCPP} include/${hpp})

        #### this is better but only supported from cmake 3.0:
        ###add_library(${libname} OBJECT ${DUMMYCPP} include/${hpp})

    endforeach()
endif()

install(DIRECTORY include/osmium DESTINATION include)

# We only have a copy of this file so we can use older boost versions which
# don't have it. We probably don't want to install it.
#install(FILES include/boost_unicode_iterator.hpp DESTINATION include)


#-----------------------------------------------------------------------------
#
#  Packaging
#
#-----------------------------------------------------------------------------

set(CPACK_PACKAGE_VERSION_MAJOR ${LIBOSMIUM_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${LIBOSMIUM_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${LIBOSMIUM_VERSION_PATCH})

if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()

include(CPack)


#-----------------------------------------------------------------------------
#
#  Print warnings at the end
#
#-----------------------------------------------------------------------------
if(BUILD_DATA_TESTS AND OSM_TESTDATA STREQUAL "OSM_TESTDATA-NOTFOUND")
    message("\n========================== WARNING ==========================")
    message("osm-testdata directory not found, data tests were disabled!\n")
    message("You can get it from https://github.com/osmcode/osm-testdata")
    message("Clone it into the same directory libosmium is in")
    message("or set the OSM_TESTDATA cmake variable to its path.")
    message("=============================================================\n")
endif()

#-----------------------------------------------------------------------------
