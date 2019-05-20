function(get_last_git_commit_hash result_name)
  find_package(Git)
  if (GIT_FOUND)
    execute_process(COMMAND
      "${GIT_EXECUTABLE}" describe --match="" --always --abbrev=40 --dirty
      WORKING_DIRECTORY "${OMIM_ROOT}"
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
      "${GIT_EXECUTABLE}" log -1 --format=%ad --date=unix
      WORKING_DIRECTORY "${OMIM_ROOT}"
      OUTPUT_VARIABLE GIT_TIMESTAMP
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  if (NOT GIT_TIMESTAMP)
    message(WARNING "Failed to get last commit from git.")
    set(GIT_TIMESTAMP "0")
  endif()

  set(${result_name} ${GIT_TIMESTAMP} PARENT_SCOPE)
endfunction()

function(configure_build_version_hpp)
  message("Configure build version ...")
  get_last_git_commit_hash(GIT_HASH)
  get_last_git_commit_timestamp(GIT_TIMESTAMP)
  configure_file("${OMIM_ROOT}/build_version.hpp.in" "${OMIM_ROOT}/build_version.hpp" @ONLY)
endfunction()


configure_build_version_hpp()
