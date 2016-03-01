# Search quality tests.

TARGET = search_quality_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
# todo(@m) revise
DEPENDENCIES = map drape_frontend routing search_tests_support search storage indexer drape \
               platform editor geometry coding base freetype expat fribidi tomcrypt gflags \
               jansson protobuf osrm stats_client minizip succinct \
               opening_hours pugixml

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src


# needed for Platform::WorkingDir() and unicode combining
QT *= core network opengl

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += \
    search_quality_tests.cpp \

HEADERS += \
