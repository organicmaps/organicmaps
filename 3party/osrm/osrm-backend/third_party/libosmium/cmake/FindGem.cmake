# Author thomas.roehr@dfki.de
#
# Version 0.3 2013-07-02
#       - rely on `gem content` to find library and header
#       - introduce GEM_OS_PKG to allow search via pkgconfig
# Version 0.2 2010-01-14
#       - add support for searching for multiple gems
# Version 0.1 2010-12-15
# 	- support basic search functionality 
#       - tested to find rice
#
# OUTPUT:
#
# GEM_INCLUDE_DIRS	After successful search contains the include directores
#
# GEM_LIBRARIES		After successful search contains the full path of each found library
#
#
# Usage: 
# set(GEM_DEBUG TRUE)
# find_package(Gem COMPONENTS rice hoe)
# include_directories(${GEM_INCLUDE_DIRS})
# target_link_libraries(${GEM_LIBRARIES}
#
# in case pkg-config should be used to search for the os pkg, set GEM_OS_PKG, i.e.
# set(GEM_OS_PKG TRUE)
#
# Check for how 'gem' should be called
include(FindPackageHandleStandardArgs)
find_program(GEM_EXECUTABLE
    NAMES "gem${RUBY_VERSION_MAJOR}${RUBY_VERSION_MINOR}"
        "gem${RUBY_VERSION_MAJOR}.${RUBY_VERSION_MINOR}"
        "gem-${RUBY_VERSION_MAJOR}${RUBY_VERSION_MINOR}"
        "gem-${RUBY_VERSION_MAJOR}.${RUBY_VERSION_MINOR}"
        "gem${RUBY_VERSION_MAJOR}${RUBY_VERSION_MINOR}${RUBY_VERSION_PATCH}"
        "gem${RUBY_VERSION_MAJOR}.${RUBY_VERSION_MINOR}.${RUBY_VERSION_PATCH}"
        "gem-${RUBY_VERSION_MAJOR}${RUBY_VERSION_MINOR}${RUBY_VERSION_PATCH}"
        "gem-${RUBY_VERSION_MAJOR}.${RUBY_VERSION_MINOR}.${RUBY_VERSION_PATCH}"
        "gem")

# Making backward compatible
if(Gem_DEBUG)
    set(GEM_DEBUG TRUE)
endif()

if(NOT GEM_EXECUTABLE)
	MESSAGE(FATAL_ERROR "Could not find the gem executable - install 'gem' first")
endif()

if(NOT Gem_FIND_COMPONENTS)
	MESSAGE(FATAL_ERROR "If searching for a Gem you have to provide COMPONENTS with the name of the gem")
endif()

foreach(Gem_NAME ${Gem_FIND_COMPONENTS})
    set(GEM_${Gem_NAME}_FOUND TRUE)
    list(APPEND components_found_vars GEM_${Gem_NAME}_FOUND)
    # If the gem is installed as a gem
    if(NOT GEM_OS_PKG)
	    set(GEM_HOME ENV{GEM_HOME})

        # Use `gem content <gem-name>` to extract current information about installed gems
        # Store the information into ${GEM_LOCAL_INFO}
        EXECUTE_PROCESS(COMMAND ${GEM_EXECUTABLE} content ${Gem_NAME}
            RESULT_VARIABLE GEM_RUN_RESULT
            OUTPUT_VARIABLE GEM_LOCAL_INFO)

        if(GEM_RUN_RESULT STREQUAL "0")
            list(APPEND FOUND_GEMS ${Gem_NAME})
            set(_library_NAME_PATTERN lib${Gem_NAME}.a
	        		   lib${Gem_NAME}.so
	        		   lib${Gem_NAME}.dylib
	        		   ${Gem_NAME}.a
	        		   ${Gem_NAME}.so
	        		   ${Gem_NAME}.dylib
                       .*.a
                       .*.so
                       .*.dylib
	        )

            set(_header_SUFFIX_PATTERN
                        .h
                        .hh
                        .hpp
            )

            # Create a list from the output results of the gem command
            string(REPLACE "\n" ";" GEM_CONTENT_LIST "${GEM_LOCAL_INFO}")
            foreach(_gem_CONTENT_PATH ${GEM_CONTENT_LIST})
                
                # Convert so that only '/' Unix path separator are being using
                # needed to do proper regex matching
                FILE(TO_CMAKE_PATH ${_gem_CONTENT_PATH} gem_CONTENT_PATH)

                # Identify library -- checking for a library in the gems 'lib' (sub)directory
                # Search for an existing library, but only within the gems folder
                foreach(_library_NAME ${_library_NAME_PATTERN})
                    STRING(REGEX MATCH ".*${Gem_NAME}.*/lib/.*${_library_NAME}$" GEM_PATH_INFO "${gem_CONTENT_PATH}")
                    if(NOT "${GEM_PATH_INFO}" STREQUAL "")
                        list(APPEND GEM_LIBRARIES ${GEM_PATH_INFO})
                        break()
                    endif()
                endforeach()

                # Identify headers
                # Checking for available headers in an include directory
                foreach(_header_PATTERN ${_header_SUFFIX_PATTERN})
                    STRING(REGEX MATCH ".*${Gem_NAME}.*/include/.*${_header_PATTERN}$" GEM_PATH_INFO "${gem_CONTENT_PATH}")
                    if(NOT "${GEM_PATH_INFO}" STREQUAL "")
                        STRING(REGEX REPLACE "(.*${Gem_NAME}.*/include/).*${_header_PATTERN}$" "\\1" GEM_PATH_INFO "${gem_CONTENT_PATH}")
                        list(APPEND GEM_INCLUDE_DIRS ${GEM_PATH_INFO})
                        break()
                    endif()
                endforeach()
            endforeach()
        else()
            set(GEM_${Gem_NAME}_FOUND FALSE)
        endif()
    else(NOT GEM_OS_PKG)
        pkg_check_modules(GEM_PKG ${Gem_NAME})
        set(GEM_${GEM_NAME}_FOUND GEM_PKG_FOUND)
        set(GEM_INCLUDE_DIRS ${GEM_PKG_INCLUDE_DIRS})
        set(GEM_LIBRARIES ${GEM_PKG_LIBRARIES} ${GEM_PKG_STATIC_LIBRARIES})
        list(APPEND GEM_LIBRARIES ${GEM_PKG_LDFLAGS} ${GEM_PKG_STATIC_LDFLAGS})
        list(APPEND GEM_LIBRARIES ${GEM_PKG_LDFLAGS_OTHER} ${GEM_PKG_STATIC_LDFLAGS_OTHER})

        if(GEM_DEBUG)
            message(STATUS "GEM_OS_PKG is defined")
            message(STATUS "GEM_INCLUDE_DIRS ${GEM_INCLUDE_DIRS}")
            message(STATUS "GEM_STATIC_LIBRARY_DIRS ${GEM_PKG_STATIC_LIBRARY_DIRS}")
            message(STATUS "GEM_LIBRARY_DIRS ${GEM_PKG_STATIC_LIBRARY_DIRS}")
            message(STATUS "GEM_STATIC_LIBRARIES ${GEM_PKG_STATIC_LIBRARIES}")
            message(STATUS "GEM_LIBRARIES ${GEM_LIBRARIES}")
        endif()
    endif()

    if(GEM_DEBUG)
		message(STATUS "${Gem_NAME} library dir: ${GEM_LIBRARIES}")
		message(STATUS "${Gem_NAME} include dir: ${GEM_INCLUDE_DIRS}")
    endif()
endforeach()

# Compact the lists
if(DEFINED GEM_LIBRARIES)
    LIST(REMOVE_DUPLICATES GEM_LIBRARIES)
endif()
if(DEFINED GEM_INCLUDE_DIRS)
    LIST(REMOVE_DUPLICATES GEM_INCLUDE_DIRS)
endif()

find_package_handle_standard_args(GEM
    REQUIRED_VARS ${components_found_vars}
    FAIL_MESSAGE "Could not find all required gems")

