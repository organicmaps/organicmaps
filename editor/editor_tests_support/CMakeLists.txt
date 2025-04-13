project(editor_tests_support)

set(SRC
  helpers.cpp
  helpers.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} editor)