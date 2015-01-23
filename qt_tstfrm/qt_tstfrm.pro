TARGET = qt_tstfrm
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

QT *= core gui widgets opengl

HEADERS += \
  test_main_loop.hpp \

SOURCES += \
  test_main_loop.cpp \


