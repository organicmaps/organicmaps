# Map library tests.

TARGET = mwm_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map traffic search storage indexer platform editor geometry coding base \
               freetype expat protobuf jansson succinct pugixml stats_client icu

include($$ROOT_DIR/common.pri)

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
  ../../testing/testingmain.cpp \
  mwm_foreach_test.cpp \
  multithread_mwm_test.cpp \
  mwm_index_test.cpp \
