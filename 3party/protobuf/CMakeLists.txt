project(protobuf)

# Disable all compile-time warnings for protobuf
add_compile_options(-w)

# protobuf config
set(protobuf_LIB_NAME libprotobuf-lite)
set(protobuf_VERSION "23.4")
set(protobuf_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protobuf)
set(protobuf_SHARED_OR_STATIC "STATIC")
set(protobuf_BUILD_TESTS OFF)
set(protobuf_INSTALL OFF)

# abseil
set(ABSL_PROPAGATE_CXX_STD ON)
set(ABSL_ROOT_DIR ${protobuf_SOURCE_DIR}/third_party/abseil-cpp)
include(protobuf/cmake/abseil-cpp.cmake)

# utf8_range
set(utf8_range_ENABLE_TESTS OFF)
set(utf8_range_ENABLE_INSTALL OFF)
add_subdirectory(${protobuf_SOURCE_DIR}/third_party/utf8_range)
include_directories(SYSTEM ${protobuf_SOURCE_DIR}/third_party/utf8_range)

include(protobuf/cmake/${protobuf_LIB_NAME}.cmake)

add_library(${PROJECT_NAME} ALIAS ${protobuf_LIB_NAME})

# Disable Unity build for protobuf
set_target_properties(${protobuf_LIB_NAME} PROPERTIES UNITY_BUILD OFF)
set(ABSL_TARGETS
  absl_check
  absl_absl_log
  absl_algorithm
  absl_base
  absl_bind_front
  absl_bits
  absl_btree
  absl_cleanup
  absl_cord
  absl_core_headers
  absl_debugging
  absl_die_if_null
  absl_dynamic_annotations
  absl_flags
  absl_flat_hash_map
  absl_flat_hash_set
  absl_function_ref
  absl_hash
  absl_layout
  absl_log_initialize
  absl_log_severity
  absl_memory
  absl_node_hash_map
  absl_node_hash_set
  absl_optional
  absl_span
  absl_status
  absl_statusor
  absl_strings
  absl_synchronization
  absl_time
  absl_time_zone
  absl_type_traits
  absl_utility
  absl_variant
)
foreach(target IN LISTS ABSL_TARGETS)
  set_target_properties(${target} PROPERTIES UNITY_BUILD OFF)
endforeach()
