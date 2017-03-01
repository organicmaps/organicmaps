# mapshot binary

ROOT_DIR = ..
DEPENDENCIES = map drape_frontend routing search storage tracking traffic routing_common indexer \
               drape partners_api platform editor geometry coding base \
               freetype expat fribidi gflags jansson protobuf osrm stats_client minizip succinct \
               pugixml opening_hours stb_image sdf_image

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

# needed for Platform::WorkingDir() and unicode combining
QT *= core network opengl

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += \
    mapshot.cpp \

HEADERS += \
