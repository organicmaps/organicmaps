# Generator binary

ROOT_DIR = ../..
#DEPENDENCIES = generator map search routing storage indexer platform editor geometry coding base \
#               osrm gflags expat tess2 jansson protobuf tomcrypt \
#               succinct stats_client pugixml

#DEPENDENCIES = generator drape map drape_frontend routing search storage indexer platform editor geometry \
#               coding base freetype gflags expat tess2 fribidi tomcrypt jansson protobuf osrm stats_client \
#               minizip succinct pugixml oauthcpp

DEPENDENCIES = drape_frontend routing search storage indexer drape map platform editor geometry \
               coding base freetype expat fribidi tomcrypt jansson protobuf osrm stats_client \
               minizip succinct pugixml tess2 gflags oauthcpp generator

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
    generator_tool.cpp \

HEADERS += \
