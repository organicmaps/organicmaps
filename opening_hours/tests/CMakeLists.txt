project(opening_hours_new_tests)

set(SRC
    parser_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
    opening_hours_new
)
