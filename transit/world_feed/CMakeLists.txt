project(world_feed)

set(SRC
    color_picker.cpp
    color_picker.hpp
    date_time_helpers.cpp
    date_time_helpers.hpp
    feed_helpers.cpp
    feed_helpers.hpp
    subway_converter.cpp
    subway_converter.hpp
    world_feed.cpp
    world_feed.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_link_libraries(
    ${PROJECT_NAME}
    generator
    drape_frontend
    shaders
    routing
    mwm_diff
    bsdiff
    tracking
    traffic
    routing_common
    transit
    descriptions
    storage
    search
    ugc
    drape
    partners_api
    web_api
    local_ads
    kml
    editor
    indexer
    metrics
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
    stats_client
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

if (PLATFORM_MAC)
    omim_link_libraries(
       ${PROJECT_NAME}
       ${Qt5Widgets_LIBRARIES}
    )
endif()

if (PLATFORM_LINUX)
    omim_link_libraries(
       ${PROJECT_NAME}
       dl
    )
endif()


link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})

omim_add_test_subdirectory(world_feed_tests)
omim_add_test_subdirectory(world_feed_integration_tests)
add_subdirectory(gtfs_converter)
