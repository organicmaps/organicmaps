TARGET = drape_gui
TEMPLATE = lib
CONFIG += staticlib

DEPENDENCIES = drape base
ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/expat/lib

HEADERS += \
    skin.hpp \
    compass.hpp \
    shape.hpp \
    layer_render.hpp

SOURCES += \
    skin.cpp \
    compass.cpp \
    shape.cpp \
    layer_render.cpp

