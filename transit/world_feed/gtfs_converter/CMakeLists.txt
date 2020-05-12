project(gtfs_converter)

include_directories(${OMIM_ROOT}/3party/gflags/src)

omim_add_executable(${PROJECT_NAME}
                    gtfs_converter.cpp)

omim_link_libraries(
    ${PROJECT_NAME}
    world_feed
    gflags
)
