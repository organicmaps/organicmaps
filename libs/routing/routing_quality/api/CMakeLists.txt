project(routing_api)

set(SRC
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

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    coding
    geometry
    routing
)
