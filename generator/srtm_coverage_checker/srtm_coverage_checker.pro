# SRTM coverage checker tool.
# Checks coverage of car roads by SRTM data.

TARGET = srtm_coverage_tool
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = generator map routing search storage indexer platform editor geometry coding base \
               osrm jansson protobuf tomcrypt succinct stats_client pugixml minizip gflags

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

QT *= core

macx-* {
    LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += \
    ../../routing/routing_integration_tests/routing_test_tools.cpp \
    srtm_coverage_checker.cpp \
