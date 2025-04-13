# Console utility to convert KMB file to KML.
project(kmb_to_kml)

set(SRC kmb_to_kml.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} kml)
