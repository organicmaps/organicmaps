project(expat)

add_definitions(-DHAVE_MEMMOVE)

add_compile_options(
  "-Wall"
)

set(
  SRC
  lib/ascii.h
  lib/expat_external.h
  lib/expat.h
  lib/internal.h
  lib/nametab.h
  lib/xmlparse.c
  lib/xmlrole.c
  lib/xmlrole.h
  lib/xmltok_impl.c
  lib/xmltok_ns.c
  lib/xmltok.c
  lib/xmltok.h
)

add_library(${PROJECT_NAME} ${SRC})
