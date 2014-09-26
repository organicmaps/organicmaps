# Routing lib unit tests

TARGET = routing_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = routing indexer platform geometry coding base osrm protobuf tomcrypt

macx-*: LIBS *= "-framework Foundation" "-framework IOKit"

include($$ROOT_DIR/common.pri)

linux*: QT *= core

SOURCES += \
  ../../testing/testingmain.cpp \
  osrm_router_test.cpp \
  vehicle_model_test.cpp \

