
TARGET = drape_frontend_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

DEPENDENCIES = drape_frontend base
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

SOURCES += \
  ../../testing/testingmain.cpp \
    memory_feature_index_tests.cpp
