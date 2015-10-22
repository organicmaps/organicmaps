project(pugixml)

cmake_minimum_required(VERSION 2.6)

option(BUILD_SHARED_LIBS "Build shared instead of static library" OFF)
set(BUILD_DEFINES "" CACHE STRING "Build defines")

# Pre-defines standard install locations on *nix systems.
include(GNUInstallDirs)
mark_as_advanced(CLEAR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR)

set(HEADERS ../src/pugixml.hpp ../src/pugiconfig.hpp)
set(SOURCES ${HEADERS} ../src/pugixml.cpp)

if(DEFINED BUILD_DEFINES)
	add_definitions(${BUILD_DEFINES})
endif()

if(BUILD_SHARED_LIBS)
    add_library(pugixml SHARED ${SOURCES})
else()
    add_library(pugixml STATIC ${SOURCES})
endif()

set_target_properties(pugixml PROPERTIES VERSION 1.6 SOVERSION 1)

install(TARGETS pugixml EXPORT pugixml-config
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT pugixml-config DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pugixml)
