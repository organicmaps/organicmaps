project(opening_hours_tests)

set(SRC opening_hours_tests.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} opening_hours)
