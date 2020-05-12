project(world_feed_tests)

set(
    SRC
    world_feed_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

omim_link_libraries(
    ${PROJECT_NAME}
    indexer
    transit
    platform
    coding
    geometry
    base
    jansson
    stats_client
    oauthcpp
    opening_hours
    world_feed
    ${LIBZ}
)

link_qt5_core(${PROJECT_NAME})
