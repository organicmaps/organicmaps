project(cppjansson)
set(SRC
  jansson_handle.cpp
  cppjansson.cpp
  jansson_handle.hpp
  cppjansson.hpp
)
omim_add_library(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    jansson::jansson
)
