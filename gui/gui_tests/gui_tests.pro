TARGET = gui_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = qt_tstfrm map gui indexer graphics storage platform geometry coding base normalize \
               expat freetype fribidi protobuf tomcrypt jansson

include($$ROOT_DIR/common.pri)

QT *= opengl gui core

win32*: LIBS *= -lopengl32
macx-*: LIBS *= "-framework IOKit"

SOURCES += \
    ../../testing/testingmain.cpp \
    gui_tests.cpp

