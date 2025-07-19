project(opening_hours_new_tests)

set(SRC
    hourminutes_test.cpp
    tokenizer_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
    opening_hours_new
)
