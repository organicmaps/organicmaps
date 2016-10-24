# A tool to check matching on mwm

ROOT_DIR = ../..

DEPENDENCIES = openlr routing search storage indexer editor platform geometry coding base protobuf \
               osrm stats_client pugixml jansson succinct gflags

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

# needed for Platform::WorkingDir() and unicode combining
QT *= core

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += \
    openlr_stat.cpp \

HEADERS += \
