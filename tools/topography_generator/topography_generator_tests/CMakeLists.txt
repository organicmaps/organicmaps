project(topography_generator_tests)

omim_add_test(${PROJECT_NAME}
  square_tests.cpp
  ../marching_squares/contours_builder.cpp
  NO_PLATFORM_INIT
)

target_link_libraries(${PROJECT_NAME} PRIVATE geometry)
