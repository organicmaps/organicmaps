TARGET = publisher
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

ROOT_DIR = ..
DEPENDENCIES = gflags bzip2 zlib jansson base coding words

include($$ROOT_DIR/common.pri)

HEADERS += \
  aard_dictionary.hpp \
  slof_indexer.hpp

SOURCES += \
  aard_dictionary.cpp \
  main.cpp \
  slof_indexer.cpp
