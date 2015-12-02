# mapshot binary

ROOT_DIR = ..
DEPENDENCIES = map drape_frontend routing search storage indexer drape platform geometry coding base \
               freetype expat fribidi tomcrypt gflags jansson protobuf osrm stats_client minizip succinct

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

# needed for Platform::WorkingDir() and unicode combining
QT *= core network

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += \
    mapshot.cpp \

HEADERS += \
