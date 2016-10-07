cmake_minimum_required(VERSION 3.2)

project(pugixml C CXX)

add_compile_options(
  "-Wall"
  "-std=c++11"
)

include_directories(src)

set(
  SRC
  src/pugixml.cpp
  src/pugixml.hpp
  src/pugiconfig.hpp
)

add_library(pugixml ${SRC})
