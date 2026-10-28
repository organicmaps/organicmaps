# Wrapper around lrelease that fails the build on warnings or unfinished
# translations. lrelease lacks a -Werror flag, and Qt < 6.10 also lacks
# -fail-on-unfinished, so we inspect its output for those diagnostics.
#
# Required variables: LRELEASE, QT_VERSION, TS, QM.

if(QT_VERSION VERSION_LESS 6.10)
  # Older supported Qt releases require -idbased for qtTrId lookups.
  set(_options -idbased)
else()
  set(_options -fail-on-unfinished)
endif()

execute_process(
  COMMAND "${LRELEASE}" ${_options} "${TS}" -qm "${QM}"
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
  file(REMOVE "${QM}")
  message(FATAL_ERROR "lrelease failed for ${TS} (exit ${_result})")
endif()

set(_combined "${_output}\n${_error_output}")

# Normalization diagnostics can report discarded translations without a Warning: prefix.
if(_combined MATCHES "[Ww]arning:|Removed plural forms|Dropped [1-9][0-9]* message")
  file(REMOVE "${QM}")
  message(FATAL_ERROR "lrelease emitted a warning for ${TS}:\n${_combined}")
endif()

# The summary line looks like "Generated 861 translation(s) (861 finished and 0 unfinished)".
# A non-zero unfinished count means a message escaped the --include all
# fallback, which signals broken upstream data.
if(QT_VERSION VERSION_LESS 6.10 AND
   (NOT _combined MATCHES "[(][0-9]+ finished and 0 unfinished[)]" OR
    _combined MATCHES "Ignored [1-9][0-9]* untranslated"))
  file(REMOVE "${QM}")
  message(FATAL_ERROR "lrelease reported unfinished translations for ${TS}:\n${_combined}")
endif()
