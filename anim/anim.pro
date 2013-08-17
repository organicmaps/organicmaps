# Animation library

TARGET = anim
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

HEADERS += \
    controller.hpp \
    task.hpp \
    angle_interpolation.hpp \
    segment_interpolation.hpp \
    anyrect_interpolation.hpp \
    value_interpolation.hpp \

SOURCES += \
    controller.cpp \
    task.cpp \
    angle_interpolation.cpp \
    segment_interpolation.cpp \
    value_interpolation.cpp \
    anyrect_interpolation.cpp \
