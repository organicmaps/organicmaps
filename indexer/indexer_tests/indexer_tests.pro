TARGET = indexer_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = indexer platform geometry coding base protobuf tomcrypt
include($$ROOT_DIR/common.pri)

QT *= core

win32 {
  LIBS *= -lShell32
  win32-g++: LIBS *= -lpthread
}
macx-*: LIBS *= "-framework Foundation"

HEADERS += \
    test_polylines.hpp \

SOURCES += \
    ../../testing/testingmain.cpp \
    cell_id_test.cpp \
    cell_coverer_test.cpp \
    test_type.cpp \
    index_builder_test.cpp \
    index_test.cpp \
    interval_index_test.cpp \
    point_to_int64_test.cpp \
    mercator_test.cpp \
    sort_and_merge_intervals_test.cpp \
    geometry_coding_test.cpp \
    scales_test.cpp \
    test_polylines.cpp \
    geometry_serialization_test.cpp \
    mwm_set_test.cpp \
    categories_test.cpp \
    visibility_test.cpp \
    checker_test.cpp \
