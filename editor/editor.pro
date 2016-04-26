# Editor specific things.

TARGET = editor
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
  changeset_wrapper.cpp \
  editor_config.cpp \
  editor_notes.cpp \
  new_feature_categories.cpp \
  opening_hours_ui.cpp \
  osm_auth.cpp \
  osm_feature_matcher.cpp \
  server_api.cpp \
  ui2oh.cpp \
  xml_feature.cpp \

HEADERS += \
  changeset_wrapper.hpp \
  editor_config.hpp \
  editor_notes.hpp \
  new_feature_categories.hpp \
  opening_hours_ui.hpp \
  osm_auth.hpp \
  osm_feature_matcher.hpp \
  server_api.hpp \
  ui2oh.hpp \
  xml_feature.hpp \
  yes_no_unknown.hpp \
