# Map library tests.

TARGET = search_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES =  search platform indexer geometry coding base

include($$ROOT_DIR/common.pri)

QT *= core

win32 {
  LIBS += -lShell32
  win32-g++: LIBS += -lpthread
}

SOURCES += \
    ../../testing/testingmain.cpp \
    keyword_matcher_test.cpp \
    keyword_lang_matcher_test.cpp \
    latlon_match_test.cpp \
    string_match_test.cpp \

HEADERS += \
    match_cost_mock.hpp \
