file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.hpp")

add_executable(osrm-mapsme ${SOURCES} ${HEADERS})
target_link_libraries(osrm-mapsme ${Boost_LIBRARIES} FINGERPRINT GITDESCRIPTION COORDLIB)
