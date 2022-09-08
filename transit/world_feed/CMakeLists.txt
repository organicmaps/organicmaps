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

target_link_libraries(${PROJECT_NAME}
  generator       # routing::transit::DeserializeFromJson
  transit
  drape_frontend  # df::ToDrapeColor, df::LoadTransitColors
  opening_hours
)

omim_add_tool_subdirectory(gtfs_converter)

omim_add_test_subdirectory(world_feed_tests)
omim_add_test_subdirectory(world_feed_integration_tests)
