# Feature segments checker tool.
# Checks for different properties of feature in mwm.

TARGET = feature_segments_checker
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = generator map routing traffic routing_common storage indexer \
               platform geometry coding base minizip succinct protobuf gflags stats_client

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

QT *= core

macx-* {
    LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += \
    feature_segments_checker.cpp \
