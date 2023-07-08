#
# Locate OSMPBF library
#
# This module defines
#  OSMPBF_FOUND        - if false, do not try to link to OSMPBF
#  OSMPBF_LIBRARIES    - full library path name
#  OSMPBF_INCLUDE_DIRS - where to find OSMPBF.hpp
#
# Note that the expected include convention is
#  #include <osmpbf/osmpbf.h>
# and not
#  #include <osmpbf.h>
#

find_path(OSMPBF_INCLUDE_DIR osmpbf/osmpbf.h
    HINTS $ENV{OSMPBF_DIR}
    PATH_SUFFIXES include
    PATHS
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /opt/local # DarwinPorts
        /opt
)

find_library(OSMPBF_LIBRARY
    NAMES osmpbf
    HINTS $ENV{OSMPBF_DIR}
    PATH_SUFFIXES lib64 lib
    PATHS
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /opt/local
        /opt
)

# Handle the QUIETLY and REQUIRED arguments and set OSMPBF_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OSMPBF DEFAULT_MSG OSMPBF_LIBRARY OSMPBF_INCLUDE_DIR)

# Copy the results to the output variables.
if(OSMPBF_FOUND)
    set(OSMPBF_INCLUDE_DIRS ${OSMPBF_INCLUDE_DIR})
    set(OSMPBF_LIBRARIES ${OSMPBF_LIBRARY})
endif()

