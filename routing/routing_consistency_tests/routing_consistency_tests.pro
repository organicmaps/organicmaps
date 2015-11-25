# This subproject implements routing consistency tests.
# This tests are launched on the whole world dataset.

TARGET = routing_consistency_test
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map routing search storage indexer platform geometry coding base osrm jansson protobuf tomcrypt succinct stats_client generator gflags

include($$ROOT_DIR/common.pri)

QT *= core

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

SOURCES += \
  ../routing_integration_tests/routing_test_tools.cpp \
  routing_consistency_tests.cpp \

HEADERS += \
