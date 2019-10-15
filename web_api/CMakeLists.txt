project(web_api)

set(
  SRC
  request_headers.cpp
  request_headers.hpp
  utils.cpp
  utils.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
