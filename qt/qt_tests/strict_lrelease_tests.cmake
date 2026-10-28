file(MAKE_DIRECTORY "${TEST_DIR}")
set(ts "${TEST_DIR}/test.ts")
set(qm "${TEST_DIR}/test.qm")

function(compile_catalog body language expected_success)
  file(WRITE "${ts}" "<TS version=\"2.1\" language=\"${language}\"><context><name></name>${body}</context></TS>")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -D "LRELEASE=${LRELEASE}" -D "QT_VERSION=${QT_VERSION}"
      -D "TS=${ts}" -D "QM=${qm}" -P "${WRAPPER}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
  )
  if(expected_success)
    if(NOT result EQUAL 0 OR NOT EXISTS "${qm}")
      message(FATAL_ERROR "A finished catalog was rejected: ${output}\n${error}")
    endif()
  elseif(result EQUAL 0 OR EXISTS "${qm}")
    message(FATAL_ERROR "A broken catalog was accepted: ${output}\n${error}")
  endif()
endfunction()

compile_catalog("<message id=\"test\"><source>Text</source><translation>Text</translation></message>" en TRUE)
compile_catalog("<message id=\"test\"><source>Text</source><translation type=\"unfinished\">Text</translation></message>" en FALSE)
compile_catalog("<message id=\"test\" numerus=\"yes\"><source>%n item</source><translation><numerusform>%n item</numerusform><numerusform>%n items</numerusform></translation></message>" ast FALSE)
