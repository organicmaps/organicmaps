# This subproject implements digital elevation model source test.
# This tests are launched on the raw SRTM dataset.

TARGET = srtm_source_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = generator map routing search storage indexer platform editor geometry coding base \
               osrm jansson protobuf tomcrypt succinct stats_client pugixml minizip

macx-*: LIBS *= "-framework IOKit" "-framework SystemConfiguration"

include($$ROOT_DIR/common.pri)

QT *= core

SOURCES += \
  ../../testing/testingmain.cpp \
  ../../routing/routing_integration_tests/routing_test_tools.cpp \
  srtm_source_tests.cpp \
