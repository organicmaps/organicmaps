# Locate STXXL library
# This module defines
#  STXXL_FOUND, if false, do not try to link to libstxxl
#  STXXL_LIBRARY
#  STXXL_INCLUDE_DIR, where to find stxxl.h
#


IF( NOT STXXL_FIND_QUIETLY )
    MESSAGE(STATUS "Looking for STXXL...")
ENDIF()

FIND_PATH(STXXL_INCLUDE_DIR stxxl.h
  HINTS
  $ENV{STXXL_DIR}
  PATH_SUFFIXES stxxl include/stxxl/stxxl include/stxxl include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local # DarwinPorts
  /opt
)

FIND_LIBRARY(STXXL_LIBRARY
  NAMES stxxl
  HINTS
  $ENV{STXXL_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local
  /opt
)

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set STXXL_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(STXXL  DEFAULT_MSG  STXXL_LIBRARY STXXL_INCLUDE_DIR)

IF( NOT STXXL_FIND_QUIETLY )
    IF( STXXL_FOUND )
        MESSAGE(STATUS "Found STXXL: ${STXXL_LIBRARY}" )
    ENDIF()
ENDIF()

MARK_AS_ADVANCED(STXXL_INCLUDE_DIR STXXL_LIBRARY)
