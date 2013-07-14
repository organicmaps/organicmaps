#-------------------------------------------------
#
# Project created by QtCreator 2013-07-13T21:17:19
#
#-------------------------------------------------

TARGET = map_server_tests
CONFIG   += console warn_on
CONFIG   -= app_bundle

TEMPLATE = app

DEPENDENCIES = qt_tstfrm map indexer graphics platform geometry coding base expat freetype fribidi protobuf tomcrypt \
               map_server_utils jansson

ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

QT *= opengl gui core

win32 {
  LIBS *= -lopengl32 -lshell32
  win32-g++: LIBS *= -lpthread
}
macx-*: LIBS *= "-framework Foundation"

SOURCES += \
      ../../testing/testingmain.cpp \
    generate_map_tests.cpp \
    utils_tests.cpp

HEADERS +=
