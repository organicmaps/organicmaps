file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.hpp")


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNDEBUG")

add_executable(osrm-mapsme ${SOURCES} ${HEADERS} "${CMAKE_SOURCE_DIR}/../../succinct/rs_bit_vector.cpp")
target_link_libraries(osrm-mapsme ${Boost_LIBRARIES} FINGERPRINT GITDESCRIPTION COORDLIB
                      debug "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libcoding.a"
                            "${CMAKE_SOURCE_DIR}/../../../../omim-build-debug/out/debug/libbase.a"
                      general "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libcoding.a"
                              "${CMAKE_SOURCE_DIR}/../../../../omim-build-release/out/release/libbase.a")
