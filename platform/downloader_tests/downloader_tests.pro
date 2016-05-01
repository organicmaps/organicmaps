TARGET = downloader_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = platform_tests_support platform coding base minizip tomcrypt jansson stats_client

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
    ../../testing/testingmain.cpp \
    downloader_test.cpp \
