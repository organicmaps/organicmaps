TARGET = drape_gui
TEMPLATE = lib
CONFIG += staticlib

DEPENDENCIES = drape base
ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

HEADERS += \
    skin.hpp \

SOURCES += \
    skin.cpp \
