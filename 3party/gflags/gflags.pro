TARGET = gflags
ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

CONFIG -= warn_on
CONFIG *= warn_off

TEMPLATE = lib
CONFIG += staticlib

INCLUDEPATH += src

unix|win32-g++ {
  QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare -Wno-missing-field-initializers
}

SOURCES += \
  src/gflags.cc \
  src/gflags_completions.cc \
  src/gflags_reporting.cc

win32-msvc* {
  SOURCES += src/windows/port.cc
}

HEADERS += \
  src/config.h \
  src/gflags/gflags.h \
  src/gflags/gflags_completions.h \
  src/windows/config.h \
  src/windows/port.h \
  src/windows/gflags/gflags.h \
  src/windows/gflags/gflags_completions.h
