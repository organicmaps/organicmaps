project(routing_api)

set(
  SRC
  api.cpp
  api.hpp
  google/google_api.cpp
  google/google_api.hpp
  google/types.cpp
  google/types.hpp
  mapbox/mapbox_api.cpp
  mapbox/mapbox_api.hpp
  mapbox/types.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  routing
  traffic
  routing_common
  transit
  storage
  indexer
  platform
  mwm_diff
  bsdiff
  geometry
  coding
  base
  icu
  jansson
  oauthcpp
  protobuf
  stats_client
  ${LIBZ}
)
