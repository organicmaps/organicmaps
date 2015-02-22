TARGET = drape_gui_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

DEPENDENCIES = drape_gui coding platform base fribidi expat
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

macx-* {
  LIBS *= "-framework CoreLocation" "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"
}

SOURCES += \
  ../../testing/testingmain.cpp \
    skin_tests.cpp
