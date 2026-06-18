project(qt_common_renderer_metal)

set(SRC
  metal_window.hpp
  metal_window.mm
  ${OMIM_ROOT}/iphone/Maps/Core/MapRendering/MetalContextFactory.h
  ${OMIM_ROOT}/iphone/Maps/Core/MapRendering/MetalContextFactory.mm
)

omim_add_library(${PROJECT_NAME} ${SRC})
add_library(om::qt::common::renderer::metal ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)
set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)

find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Qt6::Gui
    drape
    om::qt::common::renderer::base
    ${METALKIT_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
)
