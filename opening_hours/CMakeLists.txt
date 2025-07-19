project(opening_hours_new)

set(SRC
    opening_hours.hpp
    opening_hours.cpp
    tokenizer.hpp
    tokenizer.cpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(tests)
