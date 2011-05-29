# -----------------------------------------------------
# Project created by Alex Zolotarev 2010-01-21T13:23:29
# -----------------------------------------------------
ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

QT       -= gui core
TARGET    = osm_unique_char_counter
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE  = app

LIBS += -L/media/ssd/alexz/omim-build-release/out/release -lcoding -lbase -lexpat

# Additional include directories
INCLUDEPATH *= ../../3party/expat/lib \
               ../../3party/boost

HEADERS += ../../coding/parse_xml.hpp \
           ../../base/string_utils.hpp \

SOURCES += main.cpp
