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


SOURCES += \
    ../../testing/testingmain.cpp \
    osm_parser_test.cpp \
    feature_merger_test.cpp \
    osm_type_test.cpp \
    osm_id_test.cpp \
    tesselator_test.cpp \
    triangles_tree_coding_test.cpp \
    coasts_test.cpp

