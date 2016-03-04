# Search quality library tests.

TARGET = search_quality_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
# todo(@m) revise
DEPENDENCIES = map drape_frontend routing search_tests_support search search_quality storage indexer drape platform geometry coding base \
               freetype expat fribidi tomcrypt gflags jansson protobuf osrm stats_client minizip succinct \
               opening_hours

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

QT *= core

macx-*: LIBS *= "-framework IOKit"

SOURCES += \
    ../../../testing/testingmain.cpp \
    sample_test.cpp \

HEADERS += \
