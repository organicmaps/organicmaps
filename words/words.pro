TARGET = words
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = bzip2 zlib base coding

include($$ROOT_DIR/common.pri)

HEADERS += \
  common.hpp \
  dictionary.hpp \
  slof.hpp \
  slof_dictionary.hpp \
  sloynik_engine.hpp \
  sloynik_index.hpp

SOURCES += \
  slof_dictionary.cpp \
  sloynik_engine.cpp \
  sloynik_index.cpp

OTHER_FILES += \
  ../bugs.txt


