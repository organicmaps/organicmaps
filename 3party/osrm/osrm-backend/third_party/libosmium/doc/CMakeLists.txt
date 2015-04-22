#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium documentation
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring documentation")

message(STATUS "Looking for doxygen")
find_package(Doxygen)

if(DOXYGEN_FOUND)
    message(STATUS "Looking for doxygen - found")
    configure_file(header.html ${CMAKE_CURRENT_BINARY_DIR}/header.html @ONLY)
    configure_file(Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE}
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
#    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html"
#            DESTINATION "share/doc/libosmium-dev")
else()
    message(STATUS "Looking for doxygen - not found")
    message(STATUS "  Disabled making of documentation.")
endif()

#-----------------------------------------------------------------------------
message(STATUS "Configuring documentation - done")


#-----------------------------------------------------------------------------
