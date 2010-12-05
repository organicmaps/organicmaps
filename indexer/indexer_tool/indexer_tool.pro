# Indexer binary

ROOT_DIR = ../..
DEPENDENCIES = map indexer platform geometry coding base gflags expat sgitess version

include($$ROOT_DIR/common.pri)

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

# needed for Platform::WorkingDir()
QT += core

SOURCES += \
  indexer_tool.cpp \
  data_generator.cpp \
  feature_generator.cpp \
  feature_sorter.cpp \
  tesselator.cpp \
  update_generator.cpp \
  grid_generator.cpp \

HEADERS += \
  osm_element.hpp \
  data_generator.hpp \
  feature_generator.hpp \
  first_pass_parser.hpp \
  data_cache_file.hpp \
  feature_sorter.hpp \
  update_generator.hpp \
  feature_bucketer.hpp \
  grid_generator.hpp \
