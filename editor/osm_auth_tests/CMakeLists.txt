project(osm_auth_tests)

set(SRC
  osm_auth_tests.cpp
  run_on_network_thread.hpp
  server_api_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} editor)
