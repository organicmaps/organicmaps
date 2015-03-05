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
    layer_render.hpp \
    ruler.hpp \
    ruler_helper.hpp \
    drape_gui.hpp \
    gui_text.hpp \
    ruler_text.hpp

SOURCES += \
    skin.cpp \
    compass.cpp \
    shape.cpp \
    layer_render.cpp \
    ruler.cpp \
    ruler_helper.cpp \
    drape_gui.cpp \
    gui_text.cpp \
    ruler_text.cpp

