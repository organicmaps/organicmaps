project(geocoder_cli)

include_directories(${OMIM_ROOT}/3party/gflags/src)

set(
  SRC
  geocoder_cli.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  geocoder
  search
  indexer
  platform
  coding
  base
  stats_client
  jansson
  ${LIBZ}
  gflags
)
