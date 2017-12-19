# Routing common lib unit tests

TARGET = transit_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = transit indexer platform coding geometry base \
               jansson stats_client

macx-*: LIBS *= "-framework IOKit" "-framework SystemConfiguration"

include($$ROOT_DIR/common.pri)
INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

QT *= core

HEADERS += \
  transit_tools.hpp \

SOURCES += \
  ../../testing/testingmain.cpp \
  transit_graph_test.cpp \
  transit_json_parsing_test.cpp \
  transit_test.cpp \
