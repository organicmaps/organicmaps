# Wrapper around lrelease that fails the build on warnings or unfinished
# translations. lrelease lacks a -Werror flag, so we run it via
# execute_process and inspect the output ourselves.
#
# Required variables: LRELEASE, TS, QM.

execute_process(
  COMMAND "${LRELEASE}" "${TS}" -qm "${QM}"
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _output
  ERROR_VARIABLE _error_output
)

# Forward lrelease's stdout/stderr to the build log so users still see progress.
if(_output)
  message(STATUS "${_output}")
endif()
if(_error_output)
  message(STATUS "${_error_output}")
endif()

if(NOT _result EQUAL 0)
  message(FATAL_ERROR "lrelease failed for ${TS} (exit ${_result})")
endif()

set(_combined "${_output}\n${_error_output}")

# Any explicit Warning: line is a failure.
if(_combined MATCHES "Warning:")
  message(FATAL_ERROR "lrelease emitted a warning for ${TS}:\n${_combined}")
endif()

# The summary line looks like "Generated 861 translation(s) (861 finished and 0 unfinished)".
# A non-zero unfinished count means a message escaped the --include all
# fallback, which signals broken upstream data.
if(NOT _combined MATCHES "0 unfinished")
  message(FATAL_ERROR "lrelease reported unfinished translations for ${TS}:\n${_combined}")
endif()
