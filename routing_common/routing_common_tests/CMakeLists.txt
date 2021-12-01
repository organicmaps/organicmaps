project(routing_common_tests)

set(SRC
  vehicle_model_for_country_test.cpp
  vehicle_model_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  indexer
  routing_common
)
