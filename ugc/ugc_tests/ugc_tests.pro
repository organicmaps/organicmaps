# UGC library tests.

TARGET = ugc_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = ugc indexer platform coding geometry base stats_client

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

include($$ROOT_DIR/common.pri)

QT *= core

SOURCES += \
  ../../testing/testingmain.cpp \
  serdes_tests.cpp \
