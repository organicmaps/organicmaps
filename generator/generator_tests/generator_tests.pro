TARGET = generator_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map generator indexer platform geometry coding base expat sgitess protobuf tomcrypt osrm

include($$ROOT_DIR/common.pri)

QT *= core

win32 {
  LIBS *= -lShell32
  win32-g++: LIBS *= -lpthread
}
macx-*: LIBS *= "-framework Foundation"

HEADERS += \

SOURCES += \
    ../../testing/testingmain.cpp \
    osm_parser_test.cpp \
    feature_merger_test.cpp \
    osm_type_test.cpp \
    osm_id_test.cpp \
    tesselator_test.cpp \
    triangles_tree_coding_test.cpp \
    coasts_test.cpp \
    feature_builder_test.cpp \
    classificator_tests.cpp \
