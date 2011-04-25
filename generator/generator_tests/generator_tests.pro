TARGET = generator_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map generator indexer platform geometry coding base expat sgitess

include($$ROOT_DIR/common.pri)

QT *= core

win32 {
  LIBS += -lShell32
  win32-g++ {
    LIBS += -lpthread
  }
}

HEADERS += \
    ../../indexer/indexer_tests/feature_routine.hpp \

SOURCES += \
    ../../testing/testingmain.cpp \
    ../../indexer/indexer_tests/feature_routine.cpp \
    feature_bucketer_test.cpp \
    osm_parser_test.cpp \
