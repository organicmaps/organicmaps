TARGET = editor_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES += editor base coding geometry opening_hours pugixml

include($$ROOT_DIR/common.pri)

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    opening_hours_ui_test.cpp \
    xml_feature_test.cpp \
