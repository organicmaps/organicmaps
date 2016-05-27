# Map library tests.

TARGET = search_integration_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..

DEPENDENCIES = search_tests_support generator_tests_support generator routing search storage \
               stats_client indexer platform editor geometry coding base tess2 protobuf \
               tomcrypt jansson succinct pugixml opening_hours

include($$ROOT_DIR/common.pri)

QT *= core

macx-*: LIBS *= "-framework IOKit"

SOURCES += \
    ../../testing/testingmain.cpp \
    generate_tests.cpp \
    helpers.cpp \
    processor_test.cpp \
    smoke_test.cpp \

HEADERS += \
    helpers.hpp \
