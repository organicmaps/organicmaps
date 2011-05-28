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
  intermediate_result.hpp \
  keyword_matcher.hpp \
  query.hpp \
  result.hpp \
  string_match.hpp \

SOURCES += \
  delimiters.cpp \
  engine.cpp \
  intermediate_result.cpp \
  keyword_matcher.cpp \
  query.cpp \
  result.cpp \
  string_match.cpp \
