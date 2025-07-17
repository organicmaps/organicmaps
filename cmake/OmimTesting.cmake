# Tests read files from a data directory.
if (NOT SKIP_TESTS)
  if (NOT IS_DIRECTORY ${CMAKE_BINARY_DIR}/data AND NOT IS_SYMLINK ${CMAKE_BINARY_DIR}/data)
    file(CREATE_LINK ${OMIM_ROOT}/data ${CMAKE_BINARY_DIR}/data SYMBOLIC)
  endif()
endif()

# TestServer fixture configuration
add_test(
  NAME OmimStartTestServer
  COMMAND start_server.py
  WORKING_DIRECTORY ${OMIM_ROOT}/tools/python/test_server
)
add_test(
  NAME OmimStopTestServer
  COMMAND stop_server.py
  WORKING_DIRECTORY ${OMIM_ROOT}/tools/python/test_server
)
set_tests_properties(OmimStartTestServer PROPERTIES FIXTURES_SETUP TestServer)
set_tests_properties(OmimStopTestServer PROPERTIES FIXTURES_CLEANUP TestServer)
set_tests_properties(OmimStartTestServer OmimStopTestServer PROPERTIES LABELS "omim-fixture")

# Options:
# * REQUIRE_QT - requires QT event loop
# * REQUIRE_SERVER - requires test server (TestServer fixture that runs testserver.py)
# * NO_PLATFORM_INIT - test doesn't require platform dependencies
# * BOOST_TEST - test is written with Boost.Test
function(omim_add_test name)
  if (SKIP_TESTS)
    return()
  endif()

  set(options REQUIRE_QT REQUIRE_SERVER NO_PLATFORM_INIT BOOST_TEST)
  cmake_parse_arguments(TEST "${options}" "" "" ${ARGN})

  set(TEST_NAME ${name})
  set(TEST_SRC ${TEST_UNPARSED_ARGUMENTS})

  omim_add_test_target(${TEST_NAME} "${TEST_SRC}" ${TEST_NO_PLATFORM_INIT} ${TEST_REQUIRE_QT} ${TEST_BOOST_TEST})
  omim_add_ctest(${TEST_NAME} ${TEST_REQUIRE_SERVER} ${TEST_BOOST_TEST})
endfunction()

function(omim_add_test_subdirectory subdir)
  if (NOT SKIP_TESTS)
    add_subdirectory(${subdir})
  else()
    message(STATUS "SKIP_TESTS: Skipping subdirectory ${subdir}")
  endif()
endfunction()

function(omim_add_test_target name src no_platform_init require_qt boost_test)
  omim_add_executable(${name}
    ${src}
    $<$<NOT:$<BOOL:${boost_test}>>:${OMIM_ROOT}/libs/testing/testingmain.cpp>
  )
  target_compile_options(${name} PRIVATE ${OMIM_WARNING_FLAGS})
  target_include_directories(${name} PRIVATE ${OMIM_INCLUDE_DIRS})

  if(no_platform_init)
    target_compile_definitions(${name} PRIVATE OMIM_UNIT_TEST_DISABLE_PLATFORM_INIT)
  else()
    target_link_libraries(${name} platform)
  endif()

  if(require_qt)
    target_compile_definitions(${name} PRIVATE OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP)
    target_link_libraries(${name} Qt6::Widgets)
  endif()

  if (NOT boost_test)
    # testingmain.cpp uses base::HighResTimer::ElapsedNano
    target_link_libraries(${name} base)
  endif()
endfunction()

function(omim_add_ctest name require_server boost_test)
  if (NOT boost_test)
    set(test_command ${name} --data_path=${OMIM_DATA_DIR} --user_resource_path=${OMIM_USER_RESOURCES_DIR})
  else()
    set(test_command ${name})
  endif()
  add_test(
    NAME ${name}
    COMMAND ${test_command}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  if (require_server)
    set_tests_properties(${name} PROPERTIES FIXTURES_REQUIRED TestServer)
  endif()
  set_tests_properties(${name} PROPERTIES LABELS "omim-test")
endfunction()
