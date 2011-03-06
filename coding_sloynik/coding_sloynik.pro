TARGET = coding_sloynik
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = bzip2 zlib base coding

include($$ROOT_DIR/common.pri)

HEADERS += \
  bzip2_compressor.hpp \
  coder.hpp \
  coder_util.hpp \
  gzip_compressor.hpp \
  polymorph_reader.hpp \
  timsort/timsort.h \

SOURCES += \
  bzip2_compressor.cpp \
  gzip_compressor.cpp \
  timsort/timsort.c \
