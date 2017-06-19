project(ugc)

set(
  SRC
  api.cpp
  api.hpp
  types.hpp
)

add_library(${PROJECT_NAME} ${SRC})
