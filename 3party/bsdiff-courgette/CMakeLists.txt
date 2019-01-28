project(bsdiff)

add_clang_compile_options("-Wno-shorten-64-to-32")

include_directories(bsdiff divsufsort)

set(
  SRC
  bsdiff/bsdiff.h
  bsdiff/bsdiff_common.h
  bsdiff/bsdiff_search.h
  divsufsort/divsufsort.cc
  divsufsort/divsufsort.h
  divsufsort/divsufsort_private.h
  divsufsort/sssort.cc
  divsufsort/trsort.cc
)

add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(bsdiff/bsdiff_tests)
