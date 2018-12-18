project(vulkan_wrapper)

add_clang_compile_options("-Wno-deprecated-declarations")
add_gcc_compile_options("-Wno-deprecated-declarations")

set(CMAKE_PREFIX_PATH ./)

include_directories(../Vulkan-Headers/include)

set(
  SRC
  vulkan_wrapper.cpp
  vulkan_wrapper.h
)

add_library(${PROJECT_NAME} ${SRC})
