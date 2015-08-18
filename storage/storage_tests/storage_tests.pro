# Storage library tests.

TARGET = storage_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = storage indexer platform_tests_support platform geometry coding base jansson tomcrypt stats_client

include($$ROOT_DIR/common.pri)

macx-*: LIBS *= "-framework IOKit"
linux*|win32-msvc*: QT *= network

QT *= core

HEADERS += \
  fake_map_files_downloader.hpp \
  task_runner.hpp \

SOURCES += \
  ../../testing/testingmain.cpp \
  country_info_test.cpp \
  fake_map_files_downloader.cpp \
  queued_country_tests.cpp \
  simple_tree_test.cpp \
  storage_tests.cpp \
  task_runner.cpp \
