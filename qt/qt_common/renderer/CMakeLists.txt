project(qt_common_renderer)

add_subdirectory(base)
if (PLATFORM_MAC)
  add_subdirectory(metal)
endif ()
add_subdirectory(opengl)

set(SRC
  renderer_factory.cpp
  renderer_factory.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
add_library(om::qt::common::renderer ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    drape
    om::qt::common::renderer::base
  PRIVATE
    om::qt::common::renderer::opengl
    om::qt::common::renderer::metal
)
