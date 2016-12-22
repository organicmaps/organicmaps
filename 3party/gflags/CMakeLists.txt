project(gflags)

include_directories(src)

set(
  SRC
  src/config.h
  src/gflags.cc
  src/gflags/gflags.h
  src/gflags/gflags_completions.h
  src/gflags_completions.cc
  src/gflags_reporting.cc
  src/windows/config.h
  src/windows/gflags/gflags.h
  src/windows/gflags/gflags_completions.h
  src/windows/port.h
)

# if PLATFORM_WINDOWS: src += src/windows/port.cc

add_library(${PROJECT_NAME} ${SRC})
