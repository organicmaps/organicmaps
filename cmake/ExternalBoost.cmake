function(run_bash)
  cmake_parse_arguments(ARG "" "CMD" "WORKING_DIRECTORY" ${ARGN})
  message("Run ${ARG_CMD} ...")
  execute_process(COMMAND bash -c ${ARG_CMD}
    WORKING_DIRECTORY ${ARG_WORKING_DIRECTORY}
    RESULT_VARIABLE ret)
  if (NOT (ret EQUAL "0"))
    message(FATAL_ERROR "Сommand ${ARG_CMD} failed with code ${ret}.")
  endif()
endfunction()

function(init_boost)
  run_bash(CMD "./bootstrap.sh" WORKING_DIRECTORY ${OMIM_BOOST_SRC})
endfunction()

function(boost_b2 option)
  run_bash(CMD "./b2 ${option}" WORKING_DIRECTORY ${OMIM_BOOST_SRC})
endfunction()

function(install_boost_headers)
  boost_b2("headers")
endfunction()

function(link_boost_lib exe library)
  boost_b2("install link=static --with-${library} --prefix=${OMIM_BOOST_BINARY_PATH}")
  find_package(Boost ${BOOST_VERSION} COMPONENTS ${library} REQUIRED)
  target_link_libraries(${exe} ${Boost_LIBRARIES})
endfunction()
