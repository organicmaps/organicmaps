# Base functions tests.

TARGET = restaurants_info
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = \
    generator search routing routing_common indexer geometry \
    editor platform coding base jansson \
    pugixml stats_client opening_hours gflags \
    oauthcpp expat protobuf \

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

QT *= core

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += \
  restaurants_info.cpp \

HEADERS +=
