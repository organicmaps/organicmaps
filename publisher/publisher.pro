TARGET = publisher
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SLOYNIK_DIR = ..
DEPENDENCIES = gflags bzip2 zlib jansson base coding coding_sloynik words

include($$SLOYNIK_DIR/sloynik_common.pri)

HEADERS += \
  aard_dictionary.hpp \
  slof_indexer.hpp

SOURCES += \
  aard_dictionary.cpp \
  main.cpp \
  slof_indexer.cpp
