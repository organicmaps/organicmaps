TARGET = qt_tstfrm
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

QT *= core gui widgets opengl

HEADERS += \
  tstwidgets.hpp \
  macros.hpp \
  gl_test_widget.hpp \
  gui_test_widget.hpp \
  test_main_loop.hpp \

SOURCES += \
  tstwidgets.cpp \
  test_main_loop.cpp \


