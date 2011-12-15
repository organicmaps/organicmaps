TARGET = qt_tstfrm
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = map yg geometry coding base

include($$ROOT_DIR/common.pri)

QT *= core gui opengl

HEADERS += \
  main_tester.hpp \
  tstwidgets.hpp \
  widgets.hpp \
  widgets_impl.hpp \
  screen_qt.hpp \
  macros.hpp

SOURCES += \
  main_tester.cpp \
  tstwidgets.cpp \
  screen_qt.cpp \


