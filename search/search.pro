# Search library.

TARGET = search
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = indexer geometry coding base

include($$ROOT_DIR/common.pri)

HEADERS += \
  delimiters.hpp \
  engine.hpp \
  keyword_matcher.hpp \
  query.hpp \
  result.hpp \
  search_processor.hpp \
  string_match.hpp \

SOURCES += \
  delimiters.cpp \
  engine.cpp \
  keyword_matcher.cpp \
  query.cpp \
  result.cpp \
  search_processor.cpp \
  string_match.cpp \
