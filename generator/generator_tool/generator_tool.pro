# Generator binary

ROOT_DIR = ../..
DEPENDENCIES = generator search routing storage indexer platform editor geometry coding base \
               osrm gflags expat tess2 jansson protobuf tomcrypt \
               succinct stats_client pugixml

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

# needed for Platform::WorkingDir() and unicode combining
QT *= core

SOURCES += \
    generator_tool.cpp \

HEADERS += \
