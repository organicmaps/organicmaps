TARGET = sqlite3_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
DEPENDENCIES = coding base sqlite3

include($$ROOT_DIR/common.pri)

win32-g++: LIBS += -lpthread

SOURCES += \
  ../../../testing/testingmain.cpp \
  smoke_test.cpp \

HEADERS +=



