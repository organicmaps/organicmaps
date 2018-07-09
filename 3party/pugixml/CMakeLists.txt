project(pugixml)

include_directories(src)

set(
  SRC
  src/pugiconfig.hpp
  src/pugixml.cpp
  src/pugixml.hpp
  src/utils.hpp
)

add_library(${PROJECT_NAME} ${SRC})
