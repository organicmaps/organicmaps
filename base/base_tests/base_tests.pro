# Base functions tests.

TARGET = base_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = base

include($$ROOT_DIR/common.pri)

DEFINES += OMIM_UNIT_TEST_DISABLE_PLATFORM_INIT

SOURCES += \
  ../../testing/testingmain.cpp \
  assert_test.cpp \
  bits_test.cpp \
  buffer_vector_test.cpp \
  bwt_tests.cpp \
  cache_test.cpp \
  clustering_map_tests.cpp \
  collection_cast_test.cpp \
  condition_test.cpp \
  containers_test.cpp \
  levenshtein_dfa_test.cpp \
  logging_test.cpp \
  newtype_test.cpp \
  math_test.cpp \
  matrix_test.cpp \
  mem_trie_test.cpp \
  move_to_front_tests.cpp \
  observer_list_test.cpp \
  range_iterator_test.cpp \
  ref_counted_tests.cpp \
  regexp_test.cpp \
  rolling_hash_test.cpp \
  scope_guard_test.cpp \
  small_set_test.cpp \
  stl_add_test.cpp \
  stl_helpers_test.cpp \
  string_format_test.cpp \
  string_utils_test.cpp \
  suffix_array_tests.cpp \
  sunrise_sunset_test.cpp \
  thread_pool_tests.cpp \
  threaded_list_test.cpp \
  threads_test.cpp \
  timegm_test.cpp \
  timer_test.cpp \
  uni_string_dfa_test.cpp \
  visitor_tests.cpp \
  worker_thread_tests.cpp \

HEADERS +=
