project(vulkan_wrapper)

set(SRC
  vulkan_wrapper.h
)

if (NOT MSVC)
  append(SRC vulkan_wrapper.cpp)
else()
  append(SRC vulkan_wrapper_win.cpp)
endif()


add_library(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC . ../Vulkan-Headers/include)

# dlopen
target_link_libraries(${PROJECT_NAME} $<$<BOOL:CMAKE_DL_LIBS>:${CMAKE_DL_LIBS}>)
