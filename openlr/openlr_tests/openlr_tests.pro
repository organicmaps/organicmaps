TARGET = openlr_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = \
  generator_tests_support \
  platform_tests_support \
  generator \
  routing \
  routing_common \
  search \
  storage \
  indexer \
  editor \
  platform_tests_support \
  platform \
  geometry \
  coding \
  base \
  protobuf \
  tess2 \
  osrm \
  stats_client \
  pugixml \
  openlr \
  jansson \
  succinct \
  icu \

include($$ROOT_DIR/common.pri)

QT *= core

macx-* {
  LIBS *= "-framework SystemConfiguration"
}

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    decoded_path_test.cpp \
