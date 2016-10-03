TARGET = partners_api_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = partners_api platform coding base tomcrypt jansson stats_client

include($$ROOT_DIR/common.pri)

QT *= core

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    booking_tests.cpp \
    uber_tests.cpp \
