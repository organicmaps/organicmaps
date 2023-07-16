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
  absl_strings
  absl_time
  absl_time_zone
)
foreach(target IN LISTS ABSL_TARGETS)
  set_target_properties(${target} PROPERTIES UNITY_BUILD OFF)
endforeach()
