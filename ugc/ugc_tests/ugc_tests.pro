# UGC library tests.

TARGET = ugc_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = ugc indexer platform coding geometry base jansson stats_client

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

QT *= core

SOURCES += \
  ../../testing/testingmain.cpp \
  serdes_binary_tests.cpp \
  serdes_tests.cpp \
  utils.cpp \

HEADERS += \
  utils.hpp \
