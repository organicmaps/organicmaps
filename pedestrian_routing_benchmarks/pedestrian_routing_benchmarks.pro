TARGET = pedestrian_routing_benchmarks
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../
DEPENDENCIES = map routing search storage indexer platform geometry coding base osrm jansson protobuf tomcrypt succinct

macx-*: LIBS *= "-framework Foundation" "-framework IOKit"

include($$ROOT_DIR/common.pri)

QT *= core

win32* : LIBS *= -lShell32

SOURCES += \
  ../testing/testingmain.cpp \
  pedestrian_routing_benchmarks.cpp \
