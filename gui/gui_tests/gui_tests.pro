TARGET = gui_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = qt_tstfrm gui map indexer graphics platform geometry coding base expat freetype fribidi protobuf

include($$ROOT_DIR/common.pri)

QT *= opengl gui core

win32 {
  LIBS *= -lopengl32 -lshell32
  win32-g++: LIBS *= -lpthread
}
macx*: LIBS *= "-framework Foundation"

SOURCES += \
    ../../testing/testingmain.cpp \
    gui_tests.cpp

