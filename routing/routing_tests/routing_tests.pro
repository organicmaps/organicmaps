# Routing lib unit tests

TARGET = routing_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = routing platform indexer geometry coding base

include($$ROOT_DIR/common.pri)

SOURCES += \
  ../../testing/testingmain.cpp \
  routing_smoke.cpp \

HEADERS +=



