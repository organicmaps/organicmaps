TARGET = graphics_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = qt_tstfrm map graphics indexer platform geometry coding base expat freetype fribidi protobuf tomcrypt

include($$ROOT_DIR/common.pri)

QT *= opengl gui core

win32*: LIBS *= -lopengl32

SOURCES += \
    ../../testing/testingmain.cpp \
    screengl_test.cpp \
    screenglglobal_test.cpp \
    shape_renderer_test.cpp \
