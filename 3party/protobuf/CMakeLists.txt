project(protobuf)

include_directories(. src ../../)

if (NOT PLATFORM_WIN)
  add_definitions(-DHAVE_PTHREAD)
endif ()

set(
  SRC
  config.h
  src/google/protobuf/descriptor_database.h
  src/google/protobuf/descriptor.h
  src/google/protobuf/dynamic_message.h
  src/google/protobuf/extension_set.cc
  src/google/protobuf/extension_set.h
  src/google/protobuf/generated_message_reflection.h
  src/google/protobuf/generated_message_util.cc
  src/google/protobuf/generated_message_util.h
  src/google/protobuf/io/coded_stream_inl.h
  src/google/protobuf/io/coded_stream.cc
  src/google/protobuf/io/coded_stream.h
  src/google/protobuf/io/tokenizer.h
  src/google/protobuf/io/zero_copy_stream_impl_lite.cc
  src/google/protobuf/io/zero_copy_stream_impl_lite.h
  src/google/protobuf/io/zero_copy_stream.cc
  src/google/protobuf/io/zero_copy_stream.h
  src/google/protobuf/message_lite.cc
  src/google/protobuf/message_lite.h
  src/google/protobuf/message.h
  src/google/protobuf/reflection_ops.h
  src/google/protobuf/repeated_field.cc
  src/google/protobuf/repeated_field.h
  src/google/protobuf/stubs/common.cc
  src/google/protobuf/stubs/common.h
  src/google/protobuf/stubs/hash.h
  src/google/protobuf/stubs/map_util.h
  src/google/protobuf/stubs/once.cc
  src/google/protobuf/stubs/once.h
  src/google/protobuf/stubs/strutil.h
  src/google/protobuf/stubs/substitute.h
  src/google/protobuf/text_format.h
  src/google/protobuf/unknown_field_set.h
  src/google/protobuf/wire_format_lite_inl.h
  src/google/protobuf/wire_format_lite.cc
  src/google/protobuf/wire_format_lite.h
  src/google/protobuf/wire_format.h
)

if (PLATFORM_LINUX OR PLATFORM_ANDROID OR (PLATFORM_WIN AND NOT MSVC))
  append(
    SRC
    src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
  )
endif()

if (PLATFORM_WIN AND MSVC)
  append(
    SRC
    src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
  )
endif()

add_library(${PROJECT_NAME} ${SRC})
