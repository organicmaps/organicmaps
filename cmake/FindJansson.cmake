# This code is commented because it is flawed now. If any system-wide include directory will be added for jannsson,
# and it will contain wrong versions of protobuf or Qt, the build will fail.
# And it is not trivial for contributors to find the root cause of such build failure.

#if (Jansson_INCLUDE_DIRS AND Jansson_LIBRARY AND Jansson_VERSION)
#  set(Jansson_FOUND TRUE)
#else ()
#  find_path(Jansson_INCLUDE_DIRS NAMES jansson.h)
#  find_library(Jansson_LIBRARY NAMES jansson)

#  if (Jansson_INCLUDE_DIRS AND Jansson_LIBRARY)
#    set(regex_jansson_version "#define[ \t]+JANSSON_VERSION[ \t]+[\"]([^\"]+)[\"]")
#    file(STRINGS "${Jansson_INCLUDE_DIRS}/jansson.h" Jansson_VERSION REGEX "${regex_jansson_version}")
#    string(REGEX REPLACE "${regex_jansson_version}" "\\1" Jansson_VERSION "${Jansson_VERSION}")
#    unset(regex_jansson_version)
#    set(Jansson_FOUND TRUE)
#  endif ()
#endif()
