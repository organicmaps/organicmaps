#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Libosmium unit tests
#
#-----------------------------------------------------------------------------

message(STATUS "Configuring unit tests")

include(CMakeParseArguments)
include_directories(include)

add_library(testlib STATIC test_main.cpp)

set(ALL_TESTS "")


#-----------------------------------------------------------------------------
#
#  Define function for adding tests
#
#  add_unit_tests(group name [ENABLE_IF bool] [LIBS libs] [LABELS labels])
#
#  group  - test group (directory)
#  name   - name of test
#  bool   - boolean variable telling whether the test should be run (optional)
#  libs   - lib or libs that should be used when compiling test (optional)
#  labels - additional labels this test should get (optional)
#
#-----------------------------------------------------------------------------
function(add_unit_test _tgroup _tname)
    set(_testid "${_tgroup}_${_tname}")
    set(_tpath "${_tgroup}/${_tname}")

    set(ALL_TESTS "${ALL_TESTS};${_tpath}" PARENT_SCOPE)

    cmake_parse_arguments(_param "" "ENABLE_IF" "LIBS;LABELS" ${ARGN})

    if(Osmium_DEBUG)
        message("${_testid} ENABLE_IF=[${_param_ENABLE_IF}] LIBS=[${_param_LIBS}] LABELS=[${_param_LABELS}]")
    endif()

    if((NOT(DEFINED _param_ENABLE_IF)) OR (_param_ENABLE_IF))
        if(Osmium_DEBUG)
            message("Adding test: ${_tpath}")
        endif()
        add_executable(${_testid} t/${_tpath}.cpp)
        target_link_libraries(${_testid} testlib)

        if(DEFINED _param_LIBS)
            if(Osmium_DEBUG)
                message("  Adding libs: ${_param_LIBS}")
            endif()
            target_link_libraries(${_testid} ${_param_LIBS})
        endif()

        add_test(NAME ${_testid}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                 COMMAND ${_testid}
        )

        set(_labels "unit;fast;${_tgroup}")
        if(DEFINED _param_LABELS)
            if(Osmium_DEBUG)
                message("  Adding labels: ${_param_LABELS}")
            endif()
            set(_labels "${_labels};${_param_LABELS}")
        endif()

        set_tests_properties(${_testid} PROPERTIES
            LABELS "${_labels}"
            ENVIRONMENT "OSMIUM_TEST_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        )
    else()
        message("Skipped test ${_tpath} because a dependency was not found")
        set(OSMIUM_SKIPPED_TESTS
            "${OSMIUM_SKIPPED_TESTS} ${_tpath}"
            CACHE STRING "Tests that were skipped because of missing dependecies")
    endif()
endfunction()

#-----------------------------------------------------------------------------
#
#  Add all tests.
#
#-----------------------------------------------------------------------------
add_unit_test(area test_area_id)
add_unit_test(area test_node_ref_segment)

add_unit_test(basic test_box)
add_unit_test(basic test_changeset)
add_unit_test(basic test_entity_bits)
add_unit_test(basic test_location)
add_unit_test(basic test_node)
add_unit_test(basic test_node_ref)
add_unit_test(basic test_object_comparisons)
add_unit_test(basic test_relation)
add_unit_test(basic test_timestamp)
add_unit_test(basic test_way)

add_unit_test(buffer test_buffer_node)
add_unit_test(buffer test_buffer_purge)

if(GEOS_FOUND AND PROJ_FOUND)
    set(GEOS_AND_PROJ_FOUND TRUE)
else()
    set(GEOS_AND_PROJ_FOUND FALSE)
endif()
add_unit_test(geom test_factory_with_projection
    ENABLE_IF ${GEOS_AND_PROJ_FOUND}
    LIBS ${GEOS_LIBRARY} ${PROJ_LIBRARY})

add_unit_test(geom test_geojson)
add_unit_test(geom test_geos ENABLE_IF ${GEOS_FOUND} LIBS ${GEOS_LIBRARY})
add_unit_test(geom test_geos_wkb ENABLE_IF ${GEOS_FOUND} LIBS ${GEOS_LIBRARY})
add_unit_test(geom test_mercator)
add_unit_test(geom test_ogr ENABLE_IF ${GDAL_FOUND} LIBS ${GDAL_LIBRARY})
add_unit_test(geom test_projection ENABLE_IF ${PROJ_FOUND} LIBS ${PROJ_LIBRARY})
add_unit_test(geom test_wkb)
add_unit_test(geom test_wkt)

add_unit_test(index test_id_to_location ENABLE_IF ${SPARSEHASH_FOUND})
add_unit_test(index test_typed_mmap)
add_unit_test(index test_typed_mmap_grow LABELS "fails_on_windows")

add_unit_test(io test_bzip2 ENABLE_IF ${BZIP2_FOUND} LIBS ${BZIP2_LIBRARIES})
add_unit_test(io test_file_formats)
add_unit_test(io test_reader LIBS "${OSMIUM_XML_LIBRARIES}")
add_unit_test(io test_output_iterator ENABLE_IF ${Threads_FOUND} LIBS ${CMAKE_THREAD_LIBS_INIT})

add_unit_test(tags test_filter)
add_unit_test(tags test_operators)
add_unit_test(tags test_tag_list)

add_unit_test(thread test_pool ENABLE_IF ${Threads_FOUND} LIBS ${CMAKE_THREAD_LIBS_INIT})

add_unit_test(util test_cast_with_assert)
add_unit_test(util test_double)
add_unit_test(util test_options)
add_unit_test(util test_string)


#-----------------------------------------------------------------------------
#
#  Check that all tests available in test/t/*/test_*.cpp are run.
#
#-----------------------------------------------------------------------------
file(GLOB TESTS_IN_DIR RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/t" t/*/test_*.cpp)

foreach(file ${TESTS_IN_DIR})
    string(REPLACE ".cpp" "" out1 ${file})
    string(REPLACE "//" "/" tname ${out1})
    list(FIND ALL_TESTS ${tname} found)
    if(${found} EQUAL -1)
        message(WARNING "Test '${tname}' not found in cmake config. It will not be run!")
    endif()
endforeach()


#-----------------------------------------------------------------------------
message(STATUS "Configuring unit tests - done")


#-----------------------------------------------------------------------------
