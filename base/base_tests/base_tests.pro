# Base functions tests.

TARGET = base_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = base

include($$ROOT_DIR/common.pri)

SOURCES += \
  ../../testing/testingmain.cpp \
  assert_test.cpp \
  bits_test.cpp \
  buffer_vector_test.cpp \
  cache_test.cpp \
  condition_test.cpp \
  const_helper.cpp \
  containers_test.cpp \
  logging_test.cpp \
  math_test.cpp \
  matrix_test.cpp \
  mem_trie_test.cpp \
  observer_list_test.cpp \
  regexp_test.cpp \
  rolling_hash_test.cpp \
  scope_guard_test.cpp \
  stl_add_test.cpp \
  string_format_test.cpp \
  string_utils_test.cpp \
  sunrise_sunset_test.cpp \
  thread_pool_tests.cpp \
  threaded_list_test.cpp \
  threads_test.cpp \
  timegm_test.cpp \
  timer_test.cpp \
  worker_thread_test.cpp \

HEADERS +=
