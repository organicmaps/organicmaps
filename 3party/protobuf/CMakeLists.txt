project(protobuf)

include_directories(. protobuf/src ../../)

if (NOT PLATFORM_WIN)
  add_definitions(-DHAVE_PTHREAD)
endif ()

add_clang_compile_options("-Wno-shorten-64-to-32")

set(
  SRC
  config.h
  protobuf/src/google/protobuf/arena.cc
  protobuf/src/google/protobuf/arenastring.cc
  protobuf/src/google/protobuf/extension_set.cc
  protobuf/src/google/protobuf/generated_message_util.cc
  protobuf/src/google/protobuf/io/coded_stream.cc
  protobuf/src/google/protobuf/io/zero_copy_stream.cc
  protobuf/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
  protobuf/src/google/protobuf/message_lite.cc
  protobuf/src/google/protobuf/repeated_field.cc
  protobuf/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
  protobuf/src/google/protobuf/stubs/bytestream.cc
  protobuf/src/google/protobuf/stubs/common.cc
  protobuf/src/google/protobuf/stubs/int128.cc
  protobuf/src/google/protobuf/stubs/once.cc
  protobuf/src/google/protobuf/stubs/status.cc
  protobuf/src/google/protobuf/stubs/statusor.cc
  protobuf/src/google/protobuf/stubs/stringpiece.cc
  protobuf/src/google/protobuf/stubs/stringprintf.cc
  protobuf/src/google/protobuf/stubs/structurally_valid.cc
  protobuf/src/google/protobuf/stubs/strutil.cc
  protobuf/src/google/protobuf/stubs/time.cc
  protobuf/src/google/protobuf/wire_format_lite.cc
)

add_library(${PROJECT_NAME} ${SRC})
