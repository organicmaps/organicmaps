# Map library tests.

TARGET = mwm_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map gui search storage graphics indexer platform anim geometry coding base \
               freetype fribidi expat protobuf tomcrypt jansson

include($$ROOT_DIR/common.pri)

QT *= core opengl

win32* {
  LIBS *= -lShell32 -lOpengl32
  win32-g++: LIBS *= -lpthread
}
macx-*: LIBS *= "-framework Foundation" "-framework IOKit"

SOURCES += \
  ../../testing/testingmain.cpp \
  mwm_foreach_test.cpp \
  multithread_mwm_test.cpp \
  mwm_index_test.cpp \
