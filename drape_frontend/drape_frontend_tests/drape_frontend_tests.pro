
TARGET = drape_frontend_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

DEPENDENCIES = drape_frontend coding platform drape base fribidi expat
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

macx-* {
  LIBS *= "-framework CoreLocation" "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"
}

SOURCES += \
  ../../testing/testingmain.cpp \
    memory_feature_index_tests.cpp \
    fribidi_tests.cpp \
    object_pool_tests.cpp \
    gui_skin_tests.cpp \
