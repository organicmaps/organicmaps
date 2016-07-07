# Map library tests.

TARGET = mwm_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map search storage indexer platform editor geometry coding base \
               freetype fribidi expat protobuf tomcrypt jansson succinct pugixml stats_client

include($$ROOT_DIR/common.pri)

QT *= core opengl

win32*: LIBS *= -lOpengl32
macx-*: LIBS *= "-framework IOKit"

SOURCES += \
  ../../testing/testingmain.cpp \
  mwm_foreach_test.cpp \
  multithread_mwm_test.cpp \
  mwm_index_test.cpp \
