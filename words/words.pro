TARGET = words
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = coding base zlib bzip2

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


