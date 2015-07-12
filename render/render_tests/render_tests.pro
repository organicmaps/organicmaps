TARGET = render_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..

DEPENDENCIES = render indexer platform geometry coding base \


include($$ROOT_DIR/common.pri)


SOURCES += \
    ../../testing/testingmain.cpp \
    feature_processor_test.cpp \
