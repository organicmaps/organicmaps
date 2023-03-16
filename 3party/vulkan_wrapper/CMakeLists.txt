project(vulkan_wrapper)

set(SRC
  vulkan_wrapper.cpp
  vulkan_wrapper.h
)

add_library(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC . ../Vulkan-Headers/include)

# dlopen
target_link_libraries(${PROJECT_NAME} $<$<BOOL:CMAKE_DL_LIBS>:${CMAKE_DL_LIBS}>)
