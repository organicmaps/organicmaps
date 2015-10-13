# Storage library tests.

TARGET = storage_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = storage indexer platform_tests_support platform geometry coding base jansson tomcrypt stats_client

include($$ROOT_DIR/common.pri)

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore"
}
win32*|linux* {
  QT *= network
}

HEADERS += \
  fake_map_files_downloader.hpp \
  task_runner.hpp \
  test_map_files_downloader.hpp \

SOURCES += \
  ../../testing/testingmain.cpp \
  country_info_getter_test.cpp \
  fake_map_files_downloader.cpp \
  queued_country_tests.cpp \
  simple_tree_test.cpp \
  storage_tests.cpp \
  task_runner.cpp \
  test_map_files_downloader.cpp \
