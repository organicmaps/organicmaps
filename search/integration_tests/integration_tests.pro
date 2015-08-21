# Map library tests.

TARGET = search_integration_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = generator routing search storage stats_client jansson indexer platform geometry coding base \
               sgitess protobuf tomcrypt opening_hours

include($$ROOT_DIR/common.pri)

QT *= core

macx-*: LIBS *= "-framework IOKit"

SOURCES += \
    ../../testing/testingmain.cpp \
    retrieval_test.cpp \
    smoke_test.cpp \
    test_mwm_builder.cpp \
    test_search_engine.cpp \
    test_search_request.cpp \

HEADERS += \
    test_mwm_builder.hpp \
    test_search_engine.hpp \
    test_search_request.hpp \
