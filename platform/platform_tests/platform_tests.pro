TARGET = platform_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = platform coding base tomcrypt

include($$ROOT_DIR/common.pri)

QT *= core

win32 {
  LIBS += -lShell32
}

macx {
  LIBS += -framework CoreLocation -framework Foundation
}


SOURCES += \
    ../../testing/testingmain.cpp \
    platform_test.cpp \

CONFIG(debug, debug|release) {
  QT *= network
  SOURCES += download_test.cpp
}
