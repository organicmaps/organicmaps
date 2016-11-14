project(fribidi)

include_directories(lib ./)

add_definitions(-DHAVE_CONFIG_H)

add_compile_options(
  "-Wall"
)

set(
  SRC
  config_android.h
  config_ios.h
  config_mac.h
  config_tizen.h
  config_win32.h
  config.h
  lib/bidi-types.h
  lib/common.h
  lib/debug.h
  lib/fribidi-arabic.c
  lib/fribidi-arabic.h
  lib/fribidi-begindecls.h
  lib/fribidi-bidi-types-list.h
  lib/fribidi-bidi-types.c
  lib/fribidi-bidi-types.h
  lib/fribidi-bidi.c
  lib/fribidi-bidi.h
  lib/fribidi-common.h
  lib/fribidi-config.h
  lib/fribidi-deprecated.c
  lib/fribidi-deprecated.h
  lib/fribidi-enddecls.h
  lib/fribidi-flags.h
  lib/fribidi-joining-types-list.h
  lib/fribidi-joining-types.c
  lib/fribidi-joining-types.h
  lib/fribidi-joining.c
  lib/fribidi-joining.h
  lib/fribidi-mem.c
  lib/fribidi-mirroring.c
  lib/fribidi-mirroring.h
  lib/fribidi-run.c
  lib/fribidi-shape.c
  lib/fribidi-shape.h
  lib/fribidi-types.h
  lib/fribidi-unicode-version.h
  lib/fribidi-unicode.h
  lib/fribidi.c
  lib/fribidi.h
  lib/joining-types.h
  lib/mem.h
  lib/run.h
)

add_library(${PROJECT_NAME} ${SRC})
