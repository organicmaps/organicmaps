# UGC library tests.

TARGET = ugc_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = generator_tests_support generator search routing routing_common ugc indexer storage editor platform \
               coding geometry base icu jansson stats_client pugixml tess2 protobuf succinct opening_hours oauthcpp

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
  storage_tests.cpp \
  utils.cpp \

HEADERS += \
  utils.hpp \
