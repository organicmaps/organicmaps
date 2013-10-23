#-------------------------------------------------
#
# Project created by QtCreator 2013-09-18T09:14:21
#
#-------------------------------------------------

TARGET = drape_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app
DEFINES += OGL_TEST_ENABLED GTEST_DONT_DEFINE_TEST

DEPENDENCIES = base gmock
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

DRAPE_DIR = ..
include($$DRAPE_DIR/drape_common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/gmock/include $$ROOT_DIR/3party/gmock/gtest/include

SOURCES += \
    glfunctions.cpp \
    testingmain.cpp \
    failure_reporter.cpp \
    glmock_functions.cpp \
    buffer_tests.cpp \
    uniform_value_tests.cpp \
    attribute_provides_tests.cpp

HEADERS += \
    glmock_functions.hpp
