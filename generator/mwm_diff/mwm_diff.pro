# Library for compact mwm diffs

TARGET = mwm_diff
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src \
               $$ROOT_DIR/3party/osrm/osrm-backend/include \
               $$ROOT_DIR/3party/jansson/src

QT *= core

SOURCES += \
    diff.cpp \

HEADERS += \
    diff.hpp \
