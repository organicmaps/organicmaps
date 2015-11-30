# Editor specific things.

TARGET = editor
TEMPLATE = lib
CONFIG += staticlib warn_on
INCLUDEPATH += ../3party/opening_hours/src

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
  ui2oh.cpp

HEADERS += \
  opening_hours_ui.hpp \
  ui2oh.hpp \
