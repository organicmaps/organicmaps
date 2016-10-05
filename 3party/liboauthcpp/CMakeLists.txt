cmake_minimum_required(VERSION 3.2)

project(oauthcpp C CXX)

add_compile_options(
  "-Wall"
  "-std=c++11"
)

include_directories(include src)

set(
  SRC
  include/liboauthcpp/liboauthcpp.h
  src/base64.cpp
  src/HMAC_SHA1.cpp
  src/SHA1.cpp
  src/urlencode.cpp
  src/liboauthcpp.cpp
)

add_library(oauthcpp ${SRC})
