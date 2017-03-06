TARGET = openlr_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = routing routing_common search storage indexer editor platform_tests_support platform \
               geometry coding base protobuf osrm stats_client pugixml openlr jansson succinct

include($$ROOT_DIR/common.pri)

QT *= core

macx-* {
  LIBS *= "-framework SystemConfiguration"
}

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    openlr_sample_test.cpp \
