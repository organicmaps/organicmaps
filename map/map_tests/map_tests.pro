# Map library tests.

TARGET = map_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map gui search storage graphics indexer platform anim geometry coding base \
               freetype fribidi expat protobuf tomcrypt jansson zlib

include($$ROOT_DIR/common.pri)

QT *= core opengl

win32* {
  LIBS *= -lShell32 -lOpengl32
  win32-g++: LIBS *= -lpthread
}
macx*: LIBS *= "-framework Foundation" "-framework IOKit"

SOURCES += \
  ../../testing/testingmain.cpp \
  kmz_unarchive_test.cpp \
  navigator_test.cpp \
# map_foreach_test.cpp \
# multithread_map_test.cpp \
  bookmarks_test.cpp \
  geourl_test.cpp \
  measurement_tests.cpp \
  mwm_url_tests.cpp \
  feature_processor_test.cpp \
  ge0_parser_tests.cpp  \
