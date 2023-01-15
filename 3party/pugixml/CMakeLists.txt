project(pugixml)

set(SRC
  pugixml/src/pugiconfig.hpp
  pugixml/src/pugixml.cpp
  pugixml/src/pugixml.hpp
)

add_library(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC pugixml/src)
