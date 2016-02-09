# Storage library tests.

TARGET = storage_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map drape_frontend routing search storage indexer drape platform_tests_support platform editor opening_hours geometry \
               coding base freetype expat fribidi tomcrypt jansson protobuf osrm stats_client \
               minizip succinct pugixml oauthcpp

include($$ROOT_DIR/common.pri)

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

HEADERS += \
  create_country_info_getter.hpp \
  fake_map_files_downloader.hpp \
  task_runner.hpp \
  test_map_files_downloader.hpp \

SOURCES += \
  ../../testing/testingmain.cpp \
  country_info_getter_test.cpp \
  create_country_info_getter.cpp \
  fake_map_files_downloader.cpp \
  queued_country_tests.cpp \
  simple_tree_test.cpp \
  storage_tests.cpp \
  task_runner.cpp \
  test_map_files_downloader.cpp \
