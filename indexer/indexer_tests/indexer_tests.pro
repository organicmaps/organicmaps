TARGET = indexer_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = qt_tstfrm yg map indexer platform geometry coding base expat sgitess freetype fribidi

include($$ROOT_DIR/common.pri)

QT *= core gui opengl

win32:LIBS += -lopengl32 -lShell32
win32-g++:LIBS += -lpthread

HEADERS += \
    feature_routine.hpp \

SOURCES += \
    ../../testing/testingmain.cpp \
    cell_covering_visualize_test.cpp \
    cell_id_test.cpp \
    cell_coverer_test.cpp \
    test_type.cpp \
    index_builder_test.cpp \
    index_test.cpp \
    interval_index_test.cpp \
    point_to_int64_test.cpp \
    mercator_test.cpp \
    sort_and_merge_intervals_test.cpp \
    feature_test.cpp \
    geometry_coding_test.cpp \
    triangles_tree_coding_test.cpp \
    feature_routine.cpp \
