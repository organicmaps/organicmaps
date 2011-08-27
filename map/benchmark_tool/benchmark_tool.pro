# Map library tests.

TARGET = map_benchmark
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map yg indexer platform geometry coding base gflags expat freetype fribidi

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
  features_loading.cpp \
    main.cpp

HEADERS += \
    api.hpp
