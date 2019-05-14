function(run_bash_command command)
  execute_process(COMMAND bash -c ${command} RESULT_VARIABLE ret)
  if (NOT (ret EQUAL "0"))
    message(FATAL_ERROR "Сommand ${command} failed with code ${ret}.")
  endif()
endfunction()

function(init_boost)
  run_bash_command("cd ${BOOST_ROOT} && ./bootstrap.sh")
endfunction()

function(install_boost option)
  run_bash_command("cd ${BOOST_ROOT} && ./b2 ${option}")
endfunction()

function(install_boost_headers)
  install_boost("headers")
endfunction()

function(link_boost_lib exe library)
  install_boost("link=static --with-${library}")
  find_package(Boost ${BOOST_VERSION} COMPONENTS ${library} REQUIRED)
  target_link_libraries(${exe} ${Boost_LIBRARIES})
endfunction()
