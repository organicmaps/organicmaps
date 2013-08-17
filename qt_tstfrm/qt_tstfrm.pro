TARGET = qt_tstfrm
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

QT *= core gui opengl

HEADERS += \
  tstwidgets.hpp \
  macros.hpp \
  gl_test_widget.hpp \
  gui_test_widget.hpp \

SOURCES += \
  tstwidgets.cpp \


