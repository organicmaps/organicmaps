# Locate OSMPBF library
# This module defines
#  OSMPBF_FOUND, if false, do not try to link to OSMPBF
#  OSMPBF_LIBRARIES
#  OSMPBF_INCLUDE_DIR, where to find OSMPBF.hpp
#
# Note that the expected include convention is
#  #include <osmpbf/osmpbf.h>
# and not
#  #include <osmpbf.h>

IF( NOT OSMPBF_FIND_QUIETLY )
    MESSAGE(STATUS "Looking for OSMPBF...")
ENDIF()

FIND_PATH(OSMPBF_INCLUDE_DIR osmpbf.h
  HINTS
  $ENV{OSMPBF_DIR}
  PATH_SUFFIXES OSMPBF include/osmpbf include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /opt/local # DarwinPorts
  /opt
)

FIND_LIBRARY(OSMPBF_LIBRARY
  NAMES osmpbf
  HINTS
  $ENV{OSMPBF_DIR}
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
# handle the QUIETLY and REQUIRED arguments and set OSMPBF_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSMPBF  DEFAULT_MSG  OSMPBF_LIBRARY OSMPBF_INCLUDE_DIR)

IF( NOT OSMPBF_FIND_QUIETLY )
    IF( OSMPBF_FOUND )
        MESSAGE(STATUS "Found OSMPBF: ${OSMPBF_LIBRARY}" )
    ENDIF()
ENDIF()

#MARK_AS_ADVANCED(OSMPBF_INCLUDE_DIR OSMPBF_LIBRARIES OSMPBF_LIBRARY OSMPBF_LIBRARY_DBG) 
