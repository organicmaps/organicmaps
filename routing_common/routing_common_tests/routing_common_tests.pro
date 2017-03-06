# Routing common lib unit tests

TARGET = routing_common_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = routing_common indexer platform_tests_support platform editor geometry coding base \
               osrm protobuf succinct jansson stats_client map traffic pugixml stats_client

macx-*: LIBS *= "-framework IOKit" "-framework SystemConfiguration"

include($$ROOT_DIR/common.pri)

QT *= core

SOURCES += \
  ../../testing/testingmain.cpp \
  vehicle_model_test.cpp \
