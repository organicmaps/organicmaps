set(DEV_SANDBOX_WAYLAND_AVAILABLE FALSE)

# Executable discovery must survive packages being removed between configures.
foreach(program PKG_CONFIG_EXECUTABLE WAYLAND_SCANNER_EXECUTABLE)
  if (DEFINED ${program} AND NOT EXISTS "${${program}}")
    unset(${program} CACHE)
    set(${program} "${program}-NOTFOUND")
  endif()
endforeach()

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  # Share GLFW's discovery results, refreshing them on every configure.
  unset(__pkg_config_checked_Wayland CACHE)
  pkg_check_modules(Wayland QUIET
    wayland-client>=0.2.7
    wayland-cursor>=0.2.7
    wayland-egl>=0.2.7
    xkbcommon>=0.5.0)
endif()

# Protocol generation runs on the build host, even when libraries use a sysroot.
find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner NO_CMAKE_FIND_ROOT_PATH)
set(scanner_status 1)
if (WAYLAND_SCANNER_EXECUTABLE)
  execute_process(COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" --version RESULT_VARIABLE scanner_status
                  OUTPUT_QUIET ERROR_QUIET TIMEOUT 5)
endif()

if (NOT PKG_CONFIG_FOUND OR NOT Wayland_FOUND OR NOT scanner_status STREQUAL "0")
  message(WARNING "Skipping dev_sandbox: Wayland development dependencies are unavailable."
                  " Install pkg-config, Wayland and xkbcommon development packages, and a runnable wayland-scanner."
                  " See docs/INSTALL.md, or disable the sandbox with BUILD_DEV_SANDBOX=OFF.")
  return()
endif()

# Normal variables override upstream cache switches in reused build directories.
set(GLFW_BUILD_WAYLAND ON)
set(GLFW_BUILD_X11 OFF)
set(DEV_SANDBOX_WAYLAND_AVAILABLE TRUE)
