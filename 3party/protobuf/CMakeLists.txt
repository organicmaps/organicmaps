project(protobuf)

# protobuf config
set(protobuf_LIB_NAME libprotobuf-lite)
set(protobuf_VERSION "24.4")
set(protobuf_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protobuf)
set(protobuf_SHARED_OR_STATIC "STATIC")
set(protobuf_BUILD_TESTS OFF)
set(protobuf_INSTALL OFF)

# abseil
set(ABSL_PROPAGATE_CXX_STD OFF)
set(ABSL_ENABLE_INSTALL OFF)
set(ABSL_FIND_GOOGLETEST OFF)
set(ABSL_ROOT_DIR ${protobuf_SOURCE_DIR}/third_party/abseil-cpp)
include(protobuf/cmake/abseil-cpp.cmake)

# utf8_range
set(utf8_range_ENABLE_TESTS OFF)
set(utf8_range_ENABLE_INSTALL OFF)
add_subdirectory(${protobuf_SOURCE_DIR}/third_party/utf8_range)

include(protobuf/cmake/${protobuf_LIB_NAME}.cmake)

add_library(${PROJECT_NAME} ALIAS ${protobuf_LIB_NAME})
target_include_directories(${protobuf_LIB_NAME} PRIVATE ${protobuf_SOURCE_DIR}/third_party/utf8_range)

# Disable Unity build for protobuf because macro in port_def.inc do not have include guard.
set_target_properties(${protobuf_LIB_NAME} PROPERTIES UNITY_BUILD OFF)
# Disable unity build to avoid name clashes in abseil.
set_target_properties(absl_strings PROPERTIES UNITY_BUILD OFF)
set_target_properties(absl_time PROPERTIES UNITY_BUILD OFF)
set_target_properties(absl_time_zone PROPERTIES UNITY_BUILD OFF)
