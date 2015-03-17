TARGET = drape_gui
TEMPLATE = lib
CONFIG += staticlib

DEPENDENCIES = drape base
ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/expat/lib

HEADERS += \
    compass.hpp \
    drape_gui.hpp \
    gui_text.hpp \
    layer_render.hpp \
    ruler.hpp \
    ruler_helper.hpp \
    shape.hpp \
    skin.hpp \

SOURCES += \
    compass.cpp \
    drape_gui.cpp \
    gui_text.cpp \
    layer_render.cpp \
    ruler.cpp \
    ruler_helper.cpp \
    shape.cpp \
    skin.cpp \

