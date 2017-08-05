TARGET = agg
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

CONFIG -= warn_on
CONFIG *= warn_off

SOURCES += \
    agg_curves.cpp \
    agg_vcgen_stroke.cpp \
