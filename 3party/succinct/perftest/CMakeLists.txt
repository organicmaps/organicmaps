file(GLOB SUCCINCT_TEST_SOURCES perftest_*.cpp)
foreach(TEST_SRC ${SUCCINCT_TEST_SOURCES})
  get_filename_component (TEST_SRC_NAME ${TEST_SRC} NAME_WE)
  add_executable(${TEST_SRC_NAME} ${TEST_SRC})
  target_link_libraries(${TEST_SRC_NAME}
    succinct
    ${Boost_LIBRARIES}
    )
endforeach(TEST_SRC)
