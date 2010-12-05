TARGET = platform_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = platform coding base

include($$ROOT_DIR/common.pri)

QT *= core network

SOURCES += \
    ../../testing/testingmain.cpp \
    platform_test.cpp \
    download_test.cpp \
