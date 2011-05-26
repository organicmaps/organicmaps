# -----------------------------------------------------
# Project created by Alex Zolotarev 2010-01-21T13:23:29
# -----------------------------------------------------
QT       -= gui core
TARGET    = osm_unique_char_counter
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE  = app

ROOT_DIR = ../..
DEPENDENCIES = coding base expat

include($$ROOT_DIR/common.pri)


# Additional include directories
INCLUDEPATH *= ../../3party/expat/lib \
               ../../3party/boost

HEADERS += ../../coding/parse_xml.hpp \
           ../../base/string_utils.hpp \

SOURCES += main.cpp
