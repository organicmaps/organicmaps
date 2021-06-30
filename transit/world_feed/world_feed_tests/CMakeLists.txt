project(world_feed_tests)

set(
    SRC
    subway_converter_tests.cpp
    world_feed_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
    ${PROJECT_NAME}
    world_feed
    generator
    transit
    drape_frontend
    shaders
    search
    routing
    mwm_diff
    bsdiff
    tracking
    traffic
    routing_common
    transit
    descriptions
    drape
    kml
    editor
    indexer
    platform
    geometry
    coding
    base
    freetype
    expat
    icu
    agg
    jansson
    protobuf
    minizip
    succinct
    pugixml
    oauthcpp
    opening_hours
    stb_image
    sdf_image
    vulkan_wrapper
    ${LIBZ}
)

if (PLATFORM_LINUX)
    omim_link_libraries(
            ${PROJECT_NAME}
            dl
    )
endif()

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})
