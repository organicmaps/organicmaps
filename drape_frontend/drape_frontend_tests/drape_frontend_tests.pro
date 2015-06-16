
TARGET = drape_frontend_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

DEPENDENCIES = drape_frontend drape platform indexer geometry coding base expat

ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

QT *= opengl

macx-* {
  LIBS *= "-framework CoreLocation" "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"
}

SOURCES += \
  ../../testing/testingmain.cpp \
  anyrect_interpolation_tests.cpp \
  memory_feature_index_tests.cpp \
  navigator_test.cpp \
  object_pool_tests.cpp \
  skin_tests.cpp \
  tile_tree_tests.cpp \
  tile_utils_tests.cpp \
  user_event_stream_tests.cpp \

