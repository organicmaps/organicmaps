TARGET = mwm_diff_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
DEPENDENCIES = mwm_diff platform coding base stats_client


include($$ROOT_DIR/common.pri)

LIBS *= -lsqlite3

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

HEADERS += \

SOURCES += \
    ../../../testing/testingmain.cpp \
    diff_test.cpp \
