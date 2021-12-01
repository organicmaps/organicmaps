project(complex_generator)

set(SRC complex_generator.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  generator
  gflags::gflags
)
