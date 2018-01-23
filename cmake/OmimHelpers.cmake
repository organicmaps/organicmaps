# Function for setting target platform:
function(omim_set_platform_var PLATFORM_VAR pattern)
  set(${PLATFORM_VAR} FALSE PARENT_SCOPE)

  if (NOT PLATFORM)
    if (${ARGN})
      list(GET ARGN 0 default_case)
      if (${default_case})
        set(${PLATFORM_VAR} TRUE PARENT_SCOPE)
        message("Setting ${PLATFORM_VAR} to true")
      endif()
    endif()
  else()
    message("Platform: ${PLATFORM}")
    if (${PLATFORM} MATCHES ${pattern})
      set(${PLATFORM_VAR} TRUE PARENT_SCOPE)
    endif()
  endif()
endfunction()

macro(find_qt5_desktop_package package)
  find_package(${package})
  if (NOT ${package}_FOUND)
    message(FATAL_ERROR "Can't find ${package}, consider to set SKIP_DESKTOP if you don't need desktop app")
  endif()
endmacro()

# Functions for using in subdirectories
function(omim_add_executable executable)
  add_executable(${executable} ${ARGN})
  if (USE_ASAN)
    target_link_libraries(${executable} "-fsanitize=address" "-fno-omit-frame-pointer")
  endif()
  if (USE_TSAN)
    target_link_libraries(${executable} "-fsanitize=thread" "-fno-omit-frame-pointer")
  endif()
  if (USE_PPROF)
    target_link_libraries(${executable} "-lprofiler")
  endif()
endfunction()

function(omim_add_test executable)
  if (NOT SKIP_TESTS)
    omim_add_executable(${executable} ${ARGN} ${OMIM_ROOT}/testing/testingmain.cpp)
  endif()
endfunction()

function(omim_add_test_subdirectory subdir)
  if (NOT SKIP_TESTS)
    add_subdirectory(${subdir})
  else()
    message("SKIP_TESTS: Skipping subdirectory ${subdir}")
  endif()
endfunction()

function(omim_add_pybindings_subdirectory subdir)
  if (PYBINDINGS)
    add_subdirectory(${subdir})
  else()
    message("Skipping pybindings subdirectory ${subdir}")
  endif()
endfunction()

function(omim_link_platform_deps target)
  if ("${ARGN}" MATCHES "platform")
    if (PLATFORM_MAC)
      target_link_libraries(
        ${target}
        "-framework CFNetwork"
        "-framework Foundation"
        "-framework IOKit"
        "-framework SystemConfiguration"
      )
    endif()
  endif()
endfunction()

function(omim_link_libraries target)
  if (TARGET ${target})
    target_link_libraries(${target} ${ARGN} ${CMAKE_THREAD_LIBS_INIT})
    omim_link_platform_deps(${target} ${ARGN})
  else()
    message("~> Skipping linking the libraries to the target ${target} as it does not exist")
  endif()
endfunction()

function(append VAR)
  set(${VAR} ${${VAR}} ${ARGN} PARENT_SCOPE)
endfunction()

function(link_opengl target)
    if (PLATFORM_MAC)
      omim_link_libraries(
        ${target}
        "-framework OpenGL"
      )
    endif()

    if (PLATFORM_LINUX)
      omim_link_libraries(
        ${target}
        ${OPENGL_gl_LIBRARY}
      )
    endif()
endfunction()

function(link_qt5_core target)
  omim_link_libraries(
    ${target}
    ${Qt5Core_LIBRARIES}
  )

  if (PLATFORM_MAC)
    omim_link_libraries(
      ${target}
      "-framework IOKit"
    )
  endif()
endfunction()

function(link_qt5_network target)
  omim_link_libraries(
    ${target}
    ${Qt5Network_LIBRARIES}
  )
endfunction()

function(link_qt5_webengine target)
  omim_link_libraries(
    ${target}
    ${Qt5WebEngineWidgets_LIBRARIES}
  )
endfunction()

function(add_clang_compile_options)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(${ARGV})
  endif()
endfunction()
