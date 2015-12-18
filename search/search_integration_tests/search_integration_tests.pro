# Map library tests.

TARGET = search_integration_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = generator_tests_support generator routing search search_tests_support storage stats_client indexer \
               platform geometry coding base tess2 protobuf tomcrypt jansson

DEPENDENCIES += opening_hours \


include($$ROOT_DIR/common.pri)

QT *= core

macx-*: LIBS *= "-framework IOKit"

SOURCES += \
    ../../testing/testingmain.cpp \
    retrieval_test.cpp \
    smoke_test.cpp \

HEADERS += \
