# Editor specific things.

TARGET = editor
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
  opening_hours_ui.cpp \
  server_api.cpp \
  ui2oh.cpp \
  xml_feature.cpp \
  osm_auth.cpp \

HEADERS += \
  opening_hours_ui.hpp \
  server_api.hpp \
  ui2oh.hpp \
  xml_feature.hpp \
  osm_auth.hpp \
