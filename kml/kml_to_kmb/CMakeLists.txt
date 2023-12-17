# Console utility to convert KML file to KMB.
project(kml_to_kmb)

set(SRC kml_to_kmb.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} kml)
