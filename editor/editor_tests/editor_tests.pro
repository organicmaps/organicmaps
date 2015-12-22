TARGET = editor_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = editor geometry coding base opening_hours pugixml

include($$ROOT_DIR/common.pri)

QT *= core

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    opening_hours_ui_test.cpp \
    xml_feature_test.cpp \
    ui2oh_test.cpp \
