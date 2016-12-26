# Protocol buffers.

TARGET = protobuf
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ./src

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
  src/google/protobuf/stubs/common.cc                   \
  src/google/protobuf/stubs/once.cc                     \
  src/google/protobuf/extension_set.cc                  \
  src/google/protobuf/generated_message_util.cc         \
  src/google/protobuf/message_lite.cc                   \
  src/google/protobuf/repeated_field.cc                 \
  src/google/protobuf/wire_format_lite.cc               \
  src/google/protobuf/io/coded_stream.cc                \
  src/google/protobuf/io/zero_copy_stream.cc            \
  src/google/protobuf/io/zero_copy_stream_impl_lite.cc  \

linux*|android*|win32-g++|tizen* {
  SOURCES += src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
}

win32-msvc* {
  SOURCES += src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
}

# Headers are always included for all configurations
HEADERS += \
  config.h \
  src/google/protobuf/descriptor.h \
  src/google/protobuf/descriptor_database.h \
  src/google/protobuf/dynamic_message.h \
  src/google/protobuf/extension_set.h \
  src/google/protobuf/generated_message_util.h \
  src/google/protobuf/generated_message_reflection.h \
  src/google/protobuf/message.h \
  src/google/protobuf/message_lite.h \
  src/google/protobuf/reflection_ops.h \
  src/google/protobuf/repeated_field.h \
  src/google/protobuf/text_format.h \
  src/google/protobuf/unknown_field_set.h \
  src/google/protobuf/wire_format.h \
  src/google/protobuf/wire_format_lite.h \
  src/google/protobuf/wire_format_lite_inl.h \
  src/google/protobuf/io/coded_stream.h \
  src/google/protobuf/io/coded_stream_inl.h \
  src/google/protobuf/io/tokenizer.h \
  src/google/protobuf/io/zero_copy_stream.h \
  src/google/protobuf/io/zero_copy_stream_impl_lite.h \
  src/google/protobuf/stubs/common.h \
  src/google/protobuf/stubs/hash.h \
  src/google/protobuf/stubs/map-util.h \
  src/google/protobuf/stubs/once.h \
  src/google/protobuf/stubs/strutil.h \
  src/google/protobuf/stubs/substitute.h \
