#-----------------------------------------------------------------------------
#
#  Helper script that runs the 'multipolygon' test.
#
#-----------------------------------------------------------------------------

# Remove files that might be left over from previous run
file(REMOVE multipolygon.db multipolygon-tests.json)


#-----------------------------------------------------------------------------
#
#  Create multipolygons from test data.
#
#-----------------------------------------------------------------------------
execute_process(
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/testdata-multipolygon
        ${OSM_TESTDATA}/grid/data/all.osm
    RESULT_VARIABLE _result
    OUTPUT_FILE multipolygon.log
    ERROR_FILE multipolygon.log
)

if(_result)
    message(FATAL_ERROR "Error running testdata-multipolygon command")
endif()


#-----------------------------------------------------------------------------
#
#  Compare created multipolygons with reference data.
#
#-----------------------------------------------------------------------------
execute_process(
    COMMAND ${RUBY} ${OSM_TESTDATA}/bin/compare-areas.rb
        ${OSM_TESTDATA}/grid/data/tests.json
        multipolygon-tests.json
    RESULT_VARIABLE _result
)

if(_result)
    message(FATAL_ERROR "Error running compare-areas command")
endif()


#-----------------------------------------------------------------------------
