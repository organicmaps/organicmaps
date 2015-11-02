TARGET = pedestrian_routing_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../
DEPENDENCIES = map routing search storage indexer platform geometry coding base \
               osrm jansson protobuf tomcrypt succinct

macx-*: LIBS *= "-framework IOKit"

include($$ROOT_DIR/common.pri)

QT *= core

SOURCES += \
  ../testing/testingmain.cpp \
  pedestrian_routing_tests.cpp \
