project(pugixml)

include_directories(src)

set(
  SRC
  src/pugixml.cpp
  src/pugixml.hpp
  src/pugiconfig.hpp
)

add_library(${PROJECT_NAME} ${SRC})
