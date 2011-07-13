# Map library tests.

TARGET = map_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map yg indexer platform geometry coding base expat freetype fribidi

include($$ROOT_DIR/common.pri)

QT *= core network

win32 {
  LIBS += -lShell32
  win32-g++ {
    LIBS += -lpthread
  }
}

!iphone*:!bada*:!android* {
  QT += opengl
}

macx|iphone* {
  LIBS += -framework Foundation
}

SOURCES += \
  ../../testing/testingmain.cpp \
  navigator_test.cpp \
  map_foreach_test.cpp \
  debug_features_test.cpp \
    draw_processor_test.cpp
