project(pugixml)

cmake_minimum_required(VERSION 2.6)

option(BUILD_SHARED_LIBS "Build shared instead of static library" OFF)
option(BUILD_TESTS "Build tests" OFF)
set(BUILD_DEFINES "" CACHE STRING "Build defines")

# Pre-defines standard install locations on *nix systems.
include(GNUInstallDirs)
mark_as_advanced(CLEAR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR)

set(HEADERS ../src/pugixml.hpp ../src/pugiconfig.hpp)
set(SOURCES ${HEADERS} ../src/pugixml.cpp)

if(DEFINED BUILD_DEFINES)
	foreach(DEFINE ${BUILD_DEFINES})
		add_definitions("-D" ${DEFINE})
	endforeach()
endif()

if(BUILD_SHARED_LIBS)
	add_library(pugixml SHARED ${SOURCES})
else()
	add_library(pugixml STATIC ${SOURCES})
endif()

# Enable C++11 long long for compilers that are capable of it
if(NOT ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} STRLESS 3.1)
	target_compile_features(pugixml PUBLIC cxx_long_long_type)
endif()

set_target_properties(pugixml PROPERTIES VERSION 1.7 SOVERSION 1)

install(TARGETS pugixml EXPORT pugixml-config 
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT pugixml-config DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pugixml)

if(BUILD_TESTS)
	file(GLOB TEST_SOURCES ../tests/*.cpp)
	file(GLOB FUZZ_SOURCES ../tests/fuzz_*.cpp)
	list(REMOVE_ITEM TEST_SOURCES ${FUZZ_SOURCES})

	add_executable(check ${TEST_SOURCES})
	target_link_libraries(check pugixml)
	add_custom_command(TARGET check POST_BUILD COMMAND check WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)
endif()