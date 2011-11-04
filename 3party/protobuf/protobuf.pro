# Protocol buffers.

TARGET = protobuf
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ./src

ROOT_DIR = ../..
DEPENDENCIES =

include($$ROOT_DIR/common.pri)

!win32:DEFINES += HAVE_PTHREAD

win32-msvc2008 {
  # Signed/unsigned comparison warning. Conversion from double to float.
  QMAKE_CXXFLAGS *= -wd4018 -wd4244 -wd4355
}

unix|win32-g++ {
  QMAKE_CXXFLAGS_WARN_ON += -Wno-unused -Wno-extra
}

SOURCES += \
  src/google/protobuf/io/coded_stream.cc \
  src/google/protobuf/stubs/common.cc \
  src/google/protobuf/extension_set.cc \
  src/google/protobuf/generated_message_util.cc \
  src/google/protobuf/message_lite.cc \
  src/google/protobuf/stubs/once.cc \
  src/google/protobuf/repeated_field.cc \
  src/google/protobuf/wire_format_lite.cc \
  src/google/protobuf/io/zero_copy_stream.cc \
  src/google/protobuf/io/zero_copy_stream_impl_lite.cc \

HEADERS += \
  config.h \
  src/google/protobuf/io/coded_stream.h \
  src/google/protobuf/io/coded_stream_inl.h \
  src/google/protobuf/stubs/common.h \
  src/google/protobuf/extension_set.h \
  src/google/protobuf/generated_message_util.h \
  src/google/protobuf/stubs/hash.h \
  src/google/protobuf/stubs/map-util.h \
  src/google/protobuf/message_lite.h \
  src/google/protobuf/stubs/once.h \
  src/google/protobuf/repeated_field.h \
  src/google/protobuf/stubs/stl_util-inl.h \
  src/google/protobuf/wire_format_lite.h \
  src/google/protobuf/wire_format_lite_inl.h \
  src/google/protobuf/io/zero_copy_stream.h \
  src/google/protobuf/io/zero_copy_stream_impl_lite.h \

