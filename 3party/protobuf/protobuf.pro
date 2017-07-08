# Protocol buffers.

TARGET = protobuf
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += .
INCLUDEPATH += ./protobuf/src

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

!win32:DEFINES += HAVE_PTHREAD

win32-msvc* {
  # Signed/unsigned comparison warning. Conversion from double to float.
  QMAKE_CXXFLAGS *= -wd4018 -wd4244 -wd4355
}

unix|win32-g++ {
  QMAKE_CXXFLAGS_WARN_ON = -w
}


# Lite runtime files are included in all platforms and devices
SOURCES += \
  protobuf/src/google/protobuf/arena.cc \
  protobuf/src/google/protobuf/arenastring.cc \
  protobuf/src/google/protobuf/extension_set.cc \
  protobuf/src/google/protobuf/generated_message_util.cc \
  protobuf/src/google/protobuf/io/coded_stream.cc \
  protobuf/src/google/protobuf/io/zero_copy_stream.cc \
  protobuf/src/google/protobuf/io/zero_copy_stream_impl_lite.cc \
  protobuf/src/google/protobuf/message_lite.cc \
  protobuf/src/google/protobuf/repeated_field.cc \
  protobuf/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc \
  protobuf/src/google/protobuf/stubs/bytestream.cc \
  protobuf/src/google/protobuf/stubs/common.cc \
  protobuf/src/google/protobuf/stubs/int128.cc \
  protobuf/src/google/protobuf/stubs/once.cc \
  protobuf/src/google/protobuf/stubs/status.cc \
  protobuf/src/google/protobuf/stubs/statusor.cc \
  protobuf/src/google/protobuf/stubs/stringpiece.cc \
  protobuf/src/google/protobuf/stubs/stringprintf.cc \
  protobuf/src/google/protobuf/stubs/structurally_valid.cc \
  protobuf/src/google/protobuf/stubs/strutil.cc \
  protobuf/src/google/protobuf/stubs/time.cc \
  protobuf/src/google/protobuf/wire_format_lite.cc \

HEADERS += \
  config.h \
  protobuf/src/google/protobuf/arena.h \
  protobuf/src/google/protobuf/arenastring.h> \
  protobuf/src/google/protobuf/extension_set.h \
  protobuf/src/google/protobuf/generated_enum_util.h \
  protobuf/src/google/protobuf/generated_message_table_driven.h \
  protobuf/src/google/protobuf/generated_message_util.h \
  protobuf/src/google/protobuf/io/coded_stream.h \
  protobuf/src/google/protobuf/message_lite.h \
  protobuf/src/google/protobuf/metadata_lite.h \
  protobuf/src/google/protobuf/repeated_field.h \
  protobuf/src/google/protobuf/stubs/common.h \
