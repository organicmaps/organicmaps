project(opening_hours_new)

set(SRC
    parser.cpp
    parser.hpp
    weekday.cpp
    weekday.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
omim_add_test_subdirectory(tests)
