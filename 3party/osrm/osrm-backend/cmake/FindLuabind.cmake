# Locate Luabind library
# This module defines
#  LUABIND_FOUND, if false, do not try to link to Luabind
#  LUABIND_LIBRARIES
#  LUABIND_INCLUDE_DIR, where to find luabind.hpp
#
# Note that the expected include convention is
#  #include <luabind/luabind.hpp>
# and not
#  #include <luabind.hpp>

IF( NOT LUABIND_FIND_QUIETLY )
    MESSAGE(STATUS "Looking for Luabind...")
ENDIF()

FIND_PATH(LUABIND_INCLUDE_DIR luabind.hpp
  HINTS
  $ENV{LUABIND_DIR}
  PATH_SUFFIXES luabind include/luabind include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local # DarwinPorts
  /opt
)

FIND_LIBRARY(LUABIND_LIBRARY
  NAMES luabind luabind09
  HINTS
  $ENV{LUABIND_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local
  /opt
)

FIND_LIBRARY(LUABIND_LIBRARY_DBG
  NAMES luabindd
  HINTS
  $ENV{LUABIND_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local
  /opt
)

IF(LUABIND_LIBRARY)
    SET( LUABIND_LIBRARIES "${LUABIND_LIBRARY}" CACHE STRING "Luabind Libraries")
ENDIF(LUABIND_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUABIND_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Luabind  DEFAULT_MSG  LUABIND_LIBRARIES LUABIND_INCLUDE_DIR)

IF( NOT LUABIND_FIND_QUIETLY )
    IF( LUABIND_FOUND )
        MESSAGE(STATUS "Found Luabind: ${LUABIND_LIBRARY}" )
    ENDIF()
    IF( LUABIND_LIBRARY_DBG )
        MESSAGE(STATUS "Luabind debug library availible: ${LUABIND_LIBRARY_DBG}")
    ENDIF()
ENDIF()

MARK_AS_ADVANCED(LUABIND_INCLUDE_DIR LUABIND_LIBRARIES LUABIND_LIBRARY LUABIND_LIBRARY_DBG)
