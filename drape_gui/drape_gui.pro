TARGET = drape_gui
TEMPLATE = lib
CONFIG += staticlib

DEPENDENCIES = drape base
ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/expat/lib

HEADERS += \
    button.hpp \
    country_status.hpp \
    country_status_helper.hpp \
    drape_gui.hpp \
    gui_text.hpp \
    layer_render.hpp \
    ruler.hpp \
    ruler_helper.hpp \
    shape.hpp \
    skin.hpp \
    compass.hpp \

SOURCES += \
    button.cpp \
    country_status.cpp \
    country_status_helper.cpp \
    drape_gui.cpp \
    gui_text.cpp \
    layer_render.cpp \
    ruler.cpp \
    ruler_helper.cpp \
    shape.cpp \
    skin.cpp \
    compass.cpp \

