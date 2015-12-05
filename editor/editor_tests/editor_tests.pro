TARGET = editor_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = editor base opening_hours

include($$ROOT_DIR/common.pri)

QT *= core

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    opening_hours_ui_test.cpp \
