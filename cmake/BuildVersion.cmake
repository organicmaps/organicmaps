function(get_last_git_commit_hash result_name)
  find_package(Git)
  if (GIT_FOUND)
    execute_process(COMMAND
      "${GIT_EXECUTABLE}" describe --match="" --always --abbrev=40 --dirty
      WORKING_DIRECTORY "${MAPSME_CURRENT_PROJECT_ROOT}"
      OUTPUT_VARIABLE GIT_HASH
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  if (NOT GIT_HASH)
    message(WARNING "Failed to get last commit from git.")
    set(GIT_HASH "")
  endif()

  set(${result_name} ${GIT_HASH} PARENT_SCOPE)
endfunction()

function(get_last_git_commit_timestamp result_name)
  find_package(Git)
  if (GIT_FOUND)
    execute_process(COMMAND
      "${GIT_EXECUTABLE}" show -s --format=%ct HEAD
      WORKING_DIRECTORY "${MAPSME_CURRENT_PROJECT_ROOT}"
      OUTPUT_VARIABLE GIT_TIMESTAMP
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  if (NOT GIT_TIMESTAMP)
    message(WARNING "Failed to get last commit from git.")
    set(GIT_TIMESTAMP "0")
  endif()

  set(${result_name} ${GIT_TIMESTAMP} PARENT_SCOPE)
endfunction()

function(get_git_tag_name result_name)
  find_package(Git)
  if (GIT_FOUND)
    execute_process(COMMAND
      "${GIT_EXECUTABLE}" git tag --points-at HEAD
      WORKING_DIRECTORY "${MAPSME_CURRENT_PROJECT_ROOT}"
      OUTPUT_VARIABLE GIT_TAG
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  if (NOT GIT_TAG)
    message(WARNING "Failed to get last commit from git.")
    set(GIT_TAG "no_tag_set")
  endif()

  set(${result_name} ${GIT_TAG} PARENT_SCOPE)
endfunction()

function(configure_build_version_hpp)
  message("Configure build version ...")
  get_last_git_commit_hash(GIT_HASH)
  get_last_git_commit_timestamp(GIT_TIMESTAMP)
  get_git_tag_name(GIT_TAG)
  configure_file("${PATH_WITH_BUILD_VERSION_HPP}/build_version.hpp.in" "${MAPSME_CURRENT_PROJECT_ROOT}/build_version.hpp" @ONLY)
endfunction()


configure_build_version_hpp()
