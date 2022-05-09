project(generator_integration_tests)

set(SRC
  features_tests.cpp
  helpers.cpp
  helpers.hpp
  mwm_playground.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} generator)
