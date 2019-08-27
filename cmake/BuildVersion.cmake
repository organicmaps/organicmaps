function(get_last_git_commit_hash result_name)
  execute_process(COMMAND
    "${GIT_EXECUTABLE}" describe --match="" --always --abbrev=40 --dirty
    WORKING_DIRECTORY "${MAPSME_CURRENT_PROJECT_ROOT}"
    OUTPUT_VARIABLE GIT_HASH
    RESULT_VARIABLE status
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (status STREQUAL "0")
    set(${result_name} ${GIT_HASH} PARENT_SCOPE)
  else()
    message(WARNING "Failed to get hash for last commit from git.")
  endif()
endfunction()

function(get_last_git_commit_timestamp result_name)
  execute_process(COMMAND
    "${GIT_EXECUTABLE}" show -s --format=%ct HEAD
    WORKING_DIRECTORY "${MAPSME_CURRENT_PROJECT_ROOT}"
    OUTPUT_VARIABLE GIT_TIMESTAMP
    RESULT_VARIABLE status
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (status STREQUAL "0")
    set(${result_name} ${GIT_TIMESTAMP} PARENT_SCOPE)
  else()
    message(WARNING "Failed to get timestamp for last commit from git.")
  endif()
endfunction()

function(get_git_tag_name result_name)
  execute_process(COMMAND
    "${GIT_EXECUTABLE}" tag --points-at HEAD
    WORKING_DIRECTORY "${MAPSME_CURRENT_PROJECT_ROOT}"
    OUTPUT_VARIABLE GIT_TAG
    RESULT_VARIABLE status
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (status STREQUAL "0")
    set(${result_name} ${GIT_TAG} PARENT_SCOPE)
  else()
    message(WARNING "Failed to get tag for last commit from git.")
  endif()
endfunction()

function(configure_build_version_hpp)
  message(STATUS "Configure build version ...")
  set(GIT_HASH "")
  set(GIT_TIMESTAMP "0")
  set(GIT_TAG "")
  find_package(Git)
  if (GIT_FOUND)
    get_last_git_commit_hash(GIT_HASH)
    get_last_git_commit_timestamp(GIT_TIMESTAMP)
    get_git_tag_name(GIT_TAG)
  endif()
  configure_file("${PATH_WITH_BUILD_VERSION_HPP}/build_version.hpp.in" "${MAPSME_CURRENT_PROJECT_ROOT}/build_version.hpp" @ONLY)
endfunction()

configure_build_version_hpp()
