# Map library tests.

TARGET = benchmark_tool
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map indexer platform geometry coding base gflags protobuf tomcrypt

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

QT *= core

win32 {
  LIBS *= -lShell32
  win32-g++: LIBS *= -lpthread
}
macx*: LIBS *= "-framework Foundation"

SOURCES += \
    features_loading.cpp \
    main.cpp \
    api.cpp \

HEADERS += \
    api.hpp \
