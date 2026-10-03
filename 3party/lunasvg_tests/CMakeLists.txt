project(lunasvg_tests)

set(SRC
  lunasvg_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE lunasvg)
