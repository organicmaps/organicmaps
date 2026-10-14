set(OMIM_WINDOWS_DISTRIBUTION "development" CACHE STRING "Windows desktop distribution channel")
set_property(CACHE OMIM_WINDOWS_DISTRIBUTION PROPERTY STRINGS development direct store)
if (NOT PLATFORM_WIN)
  if (NOT OMIM_WINDOWS_DISTRIBUTION STREQUAL "development")
    message(FATAL_ERROR "Windows distribution options can only be used on Windows")
  endif()
  return()
endif()

if (NOT OMIM_WINDOWS_DISTRIBUTION MATCHES "^(development|direct|store)$")
  message(FATAL_ERROR "Invalid OMIM_WINDOWS_DISTRIBUTION: ${OMIM_WINDOWS_DISTRIBUTION}")
endif()

set(OMIM_WINDOWS_VERSION_NUMBERS "0,0,0,0")
set(OMIM_WINDOWS_VERSION_STRING "Development")
if (NOT OMIM_WINDOWS_DISTRIBUTION STREQUAL "development")
  if (BUILD_DESIGNER OR SKIP_QT_GUI)
    message(FATAL_ERROR "Windows distribution requires the desktop application and BUILD_DESIGNER=OFF")
  endif()
  if (WITH_SYSTEM_PROVIDED_3PARTY)
    message(FATAL_ERROR "Windows distribution currently requires bundled third-party libraries")
  endif()
  get_property(_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if (NOT _multi_config AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR "Windows distribution requires a Release build")
  endif()
  find_program(BASH bash REQUIRED HINTS "$ENV{ProgramFiles}/Git/bin")
  execute_process(
    COMMAND "${BASH}" tools/unix/version.sh windows_version
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE OMIM_WINDOWS_VERSION_STRING
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
  )
  string(REPLACE "." "," OMIM_WINDOWS_VERSION_NUMBERS "${OMIM_WINDOWS_VERSION_STRING}")
endif()
