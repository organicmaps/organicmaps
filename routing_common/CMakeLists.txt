project(routing_common)

include_directories(
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  bicycle_model.cpp
  bicycle_model.hpp
  car_model.cpp
  car_model.hpp
  num_mwm_id.hpp
  pedestrian_model.cpp
  pedestrian_model.hpp
  transit_graph_data.cpp
  transit_graph_data.hpp
  transit_serdes.hpp
  transit_speed_limits.hpp
  transit_types.cpp
  transit_types.hpp
  vehicle_model.cpp
  vehicle_model.hpp
)

add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(routing_common_tests)
