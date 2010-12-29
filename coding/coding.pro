# Coding project.
TARGET = coding
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = base

include($$ROOT_DIR/common.pri)

SOURCES += \
  hex.cpp \
  file_reader.cpp \
  file_writer.cpp \
  lodepng.cpp \
  file_container.cpp \


HEADERS += \
  internal/xmlparser.h \
  internal/expat_impl.h \
  internal/file_data.hpp \
  internal/file64_api.hpp \
  parse_xml.hpp \
  varint.hpp \
  mm_vector.hpp \
  mm_bit_vector.hpp \
  mm_base.hpp \
  endianness.hpp \
  byte_stream.hpp \
  var_serial_vector.hpp \
  hex.hpp \
  mm_compact_trie.hpp \
  mm_compact_tree.hpp \
  compact_trie_builder.hpp \
  compact_tree_builder.hpp \
  bit_vector_builder.hpp \
  dd_vector.hpp \
  dd_bit_vector.hpp \
  dd_base.hpp \
  strutil.hpp \
  writer.hpp \
  write_to_sink.hpp \
  reader.hpp \
  dd_bit_rank_directory.hpp \
  dd_compact_tree.hpp \
  dd_compact_trie.hpp \
  diff.hpp \
  diff_patch_common.hpp \
  source.hpp \
  lodepng.hpp \
  lodepng_io.hpp \
  lodepng_io_private.hpp \
  var_record_reader.hpp \
  file_sort.hpp \
  file_reader.hpp \
  file_writer.hpp \
  reader_cache.hpp \
  buffer_reader.hpp \
  streams.hpp \
  streams_sink.hpp \
  streams_common.hpp \
  file_container.hpp \
